/*
 *  Copyright (C) 2021-2022 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

/* Standard includes. */
#include <stdlib.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "osal/src/nonos/Nonos_config.h"

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
	/* Check the configuration. */
	#if( configMAX_PRIORITIES > 32 )
		#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
	#endif
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

#if configUSE_TASK_FPU_SUPPORT > 2
	#error configUSE_TASK_FPU_SUPPORT must be set to 1, 2, or left undefined.
#endif

#if configUSE_TASK_FPU_SUPPORT == 0
	#error configUSE_TASK_FPU_SUPPORT must not be set to 0
#endif

/*
 * A critical section is exited when the critical section nesting count reaches
 * this value.
 */
#define portNO_CRITICAL_NESTING			( ( size_t ) 0 )

/*
 * In all GICs 255 can be written to the priority mask register to unmask all
 * (but the lowest) interrupt priority.
 */
#define portUNMASK_VALUE				( 0xFFUL )

/*
 * Tasks are not created with a floating point context, but can be given a
 * floating point context after they have been created.  A variable is stored as
 * part of the tasks context that holds portNO_FLOATING_POINT_CONTEXT if the task
 * does not have an FPU context, or any other value if the task does have an FPU
 * context.
 */
#define portNO_FLOATING_POINT_CONTEXT	( ( StackType_t ) 0 )

/* Constants required to setup the initial task context. */
#define portSP_ELx						( ( StackType_t ) 0x01 )
#define portSP_EL0						( ( StackType_t ) 0x00 )

#define portEL1							( ( StackType_t ) 0x04 )
#define portINITIAL_PSTATE				( portEL1 | portSP_EL0 )

/*
 * Let the user override the pre-loading of the initial LR with the address of
 * prvTaskExitError() in case is messes up unwinding of the stack in the
 * debugger.
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif
/*-----------------------------------------------------------*/

/*
 * Starts the first task executing. This function is necessarily written in
 * assembly code so is implemented in portASM.s.
 */
extern void vPortRestoreTaskContext( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );
/*-----------------------------------------------------------*/

/*
 * A variable is used to keep track of the critical section nesting.  This
 * variable has to be stored as part of the task context and must be initialised to
 * a non zero value to ensure interrupts don't inadvertently become unmasked before
 * the scheduler starts.  As it is stored as part of the task context it will
 * automatically be set to 0 when the first task is started.
 */
volatile uint64_t ullCriticalNesting = 9999ULL;

/*
 * Saved as part of the task context.  If ullPortTaskHasFPUContext is non-zero
 * then floating point context must be saved and restored for the task.
 */
uint64_t ullPortTaskHasFPUContext = pdFALSE;

/* Set to 1 to pend a context switch from an ISR. */
uint64_t ullPortYieldRequired = pdFALSE;

/*
 * Counts the interrupt nesting depth.  A context switch is only performed if
 * if the nesting depth is 0.
 */
uint64_t ullPortInterruptNesting = 0;

/* Set to true when schedular gets enabled in xPortStartScheduler */
uint32_t ulPortSchedularRunning = pdFALSE;

/*
 * Run Time Timer Control Structure. 
 */
typedef struct xRUN_TIME_TIMER_CONTROL 
{
    uint64_t        uxTicks;
    TimerP_Handle   pxTimerHandle;
    uint32_t        ulUSecPerTick;
    uint32_t        ulTimerReloadCnt; 
} RunTimeTimerControl_t;

RunTimeTimerControl_t   gTimerCntrl;

/*
 * Starts the first task executing. This function is necessarily written in
 * assembly code so is implemented in portASM.s.
 */
extern void vPortRestoreTaskContext( void );

uint64_t uxPortGetTimeInUsec(void);

/*
 * The space on the stack required to hold the FPU registers. This is 32 128-bit
 * registers, that means (64 * 8) 64 double words.
 */
#define portFPU_REGISTER_DOUBLE_WORDS ( 64 )

__attribute__(( used )) const uint64_t ullMaxAPIPriorityMask = ( portUNMASK_VALUE );

/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    /*
	 * A function that implements a task must not exit or attempt to return to
     * its caller as there is nothing to return to.  If a task wants to exit it
     * should instead call vTaskDelete( NULL ).
     *
     * Force an assert() to be triggered if configASSERT() is
     * defined, then stop here so application writers can catch the error.
	 */
    DebugP_assert(0);
}

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, 
									TaskFunction_t pxCode, 
									void *pvParameters )
{
	/*
	 * Setup the initial stack of the task.  The stack is set exactly as
	 * expected by the portRESTORE_CONTEXT() macro.
	 */

	/* First all the general purpose registers. */
	pxTopOfStack--;
	*pxTopOfStack = 0x0101010101010101ULL;	/* R1 */
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) pvParameters; /* R0 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0303030303030303ULL;	/* R3 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0202020202020202ULL;	/* R2 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0505050505050505ULL;	/* R5 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0404040404040404ULL;	/* R4 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0707070707070707ULL;	/* R7 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0606060606060606ULL;	/* R6 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0909090909090909ULL;	/* R9 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0808080808080808ULL;	/* R8 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1111111111111111ULL;	/* R11 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1010101010101010ULL;	/* R10 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1313131313131313ULL;	/* R13 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1212121212121212ULL;	/* R12 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1515151515151515ULL;	/* R15 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1414141414141414ULL;	/* R14 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1717171717171717ULL;	/* R17 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1616161616161616ULL;	/* R16 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1919191919191919ULL;	/* R19 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1818181818181818ULL;	/* R18 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2121212121212121ULL;	/* R21 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2020202020202020ULL;	/* R20 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2323232323232323ULL;	/* R23 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2222222222222222ULL;	/* R22 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2525252525252525ULL;	/* R25 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2424242424242424ULL;	/* R24 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2727272727272727ULL;	/* R27 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2626262626262626ULL;	/* R26 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2929292929292929ULL;	/* R29 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2828282828282828ULL;	/* R28 */
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x00;	/* XZR - has no effect, used so there are an even number of registers. */
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS;	/* R30 - procedure call link register. */
	pxTopOfStack--;

	*pxTopOfStack = portINITIAL_PSTATE;
	pxTopOfStack--;

	*pxTopOfStack = ( StackType_t ) pxCode; /* Exception return address. */
	pxTopOfStack--;
#if ( configUSE_TASK_FPU_SUPPORT == 1 )
	{
		/*
		 * The task will start with a critical nesting count of 0 as interrupts are
		 * enabled.
		 */
		*pxTopOfStack = portNO_CRITICAL_NESTING;
		/*
		 * The task will start without a floating point context. A task that uses
		 * the floating point hardware must call vPortTaskUsesFPU() before executing
		 * any floating point instructions.
		 */
	 	pxTopOfStack--;
		*pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;
	}
#elif ( configUSE_TASK_FPU_SUPPORT == 2 )
	{
		/*
		 * The task will start with a floating point context. Leave enough
		 * space for the registers - and ensure they are initialised to 0.
		 */
		pxTopOfStack -= ( portFPU_REGISTER_DOUBLE_WORDS - 1 );
		memset( pxTopOfStack, 0x00, portFPU_REGISTER_DOUBLE_WORDS * sizeof( StackType_t ) );

		/*
		 * The task will start with a critical nesting count of 0 as interrupts are
		 * enabled.
		 */
		pxTopOfStack--;
		*pxTopOfStack = portNO_CRITICAL_NESTING;
		pxTopOfStack--;
		*pxTopOfStack = pdTRUE;
		ullPortTaskHasFPUContext = pdTRUE;
	}
#endif

	return pxTopOfStack;
}

__attribute__((__weak__)) void vPortGTimerCheck(void)
{
	return;
}

static void prvPorttimerTickIsr(uintptr_t args)
{
    void vPortTimerTickHandler();

    /* Increment the systick counter */
    gTimerCntrl.uxTicks++;

    vPortTimerTickHandler();

    vPortGTimerCheck();
}

static void prvPortInitTickTimer(void)
{
    TimerP_Params timerParams;
    TimerP_Handle pTickTimerHandle;

    TimerP_Params_init(&timerParams);
    timerParams.runMode    = TimerP_RunMode_CONTINUOUS;
    timerParams.startMode  = TimerP_StartMode_USER;
    timerParams.periodType = TimerP_PeriodType_MICROSECS;
    timerParams.period     = (portTICK_PERIOD_MS * 1000);

    pTickTimerHandle = TimerP_create(configTIMER_ID, &prvPorttimerTickIsr, &timerParams);

    DebugP_assert( pTickTimerHandle != NULL );

    /* Initialize internal data structure */
    gTimerCntrl.uxTicks             = 0;
    gTimerCntrl.ulUSecPerTick       = (portTICK_PERIOD_MS * 1000);
    gTimerCntrl.ulTimerReloadCnt    = TimerP_getReloadCount(pTickTimerHandle);
    gTimerCntrl.pxTimerHandle       = pTickTimerHandle;
}

static void prvPortStartTickTimer(void)
{
    TimerP_Status status;
    status = TimerP_start(gTimerCntrl.pxTimerHandle);

    DebugP_assert( status == TimerP_OK );
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
	/*
	 * Interrupts are turned off in the CPU itself to ensure tick does
     * not execute	while the scheduler is being started.  Interrupts are
     * automatically turned back on in the CPU when the first task starts
     * executing.
     */
    portDISABLE_INTERRUPTS();

    /* Start the ISR handling of the timer that generates the tick ISR. */
    ulPortSchedularRunning = pdTRUE;

    prvPortInitTickTimer();
    prvPortStartTickTimer();
    /* Start the first task executing. */
    vPortRestoreTaskContext();

    /*
	 * Will only get here if vTaskStartScheduler() was called with the CPU in
     * a non-privileged mode or the binary point register was not set to its lowest
     * possible value.  prvTaskExitError() is referenced to prevent a compiler
     * warning about it being defined but not referenced in the case that the user
     * defines their own exit address.
	 */
    ( void ) prvTaskExitError;

    return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/*
	 * Not implemented in ports where there is nothing to return to.
	 * Artificially force an assert.
	 */
	configASSERT( ullCriticalNesting == 1000ULL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
	/* TODO: Mask interrupts up to the max syscall interrupt priority. */
	__asm volatile ( "DSB SY" );
	__asm volatile ( "ISB SY" );
	__asm volatile ( "MSR DAIFSET, #3" ::: "memory" );
	__asm volatile ( "DSB SY" );
	__asm volatile ( "ISB SY" );

	/*
	 * Now interrupts are disabled ullCriticalNesting can be accessed
	 * directly.  Increment ullCriticalNesting to keep a count of how many times
	 * portENTER_CRITICAL() has been called.
	 */
	ullCriticalNesting++;
	__asm volatile ( "DSB SY" );
	__asm volatile ( "ISB SY" );

#if ( configOPTIMIZE_FOR_LATENCY == 0 )
    /*
     * This API should NOT be called from within ISR context. Below logic checks for this.
     * Commenting this reduces task switch latency a bit, however if this API is by mistale
     * called in a ISR by user, it could have unexpected side effects.
     */
    /*
     * This is not the interrupt safe version of the enter critical function so
     * assert() if it is being called from an interrupt context.  Only API
     * functions that end in "FromISR" can be used in an interrupt.  Only assert if
     * the critical nesting count is 1 to protect against recursive calls if the
     * assert function also uses a critical section.
	 */
    if ( ullCriticalNesting == 1 )
    {
        DebugP_assert( ullPortInterruptNesting == 0);
    }
#endif
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	/* Make sure critical nesting count is non zero */
	DebugP_assert( ullCriticalNesting > portNO_CRITICAL_NESTING );

	{
		/*
		 * Decrement the nesting count as the critical section is being
		 * exited.
		 */
		__asm volatile ( "DSB SY" );
		__asm volatile ( "ISB SY" );
		ullCriticalNesting--;
		__asm volatile ( "DSB SY" );
		__asm volatile ( "ISB SY" );

		/*
		 * If the nesting level has reached zero then all interrupt
		 * priorities must be re-enabled.
		 */
		if ( ullCriticalNesting == portNO_CRITICAL_NESTING )
		{
			/*
			 * Critical nesting has reached zero so all interrupt priorities
			 * should be unmasked.
			 */
			__asm volatile ( "MSR DAIFCLR, #3" ::: "memory" );
			__asm volatile ( "DSB SY" );
			__asm volatile ( "ISB SY" );
		}
	}
}
/*-----------------------------------------------------------*/

/* Initialize high resolution timer for CPU and task load calculation */
void vPortConfigTimerForRunTimeStats()
{
    /* Timer is initialized by prvPortInitTickTimer before the schedular is started */
}

/* Return current counter value of high speed counter in usecs */
uint32_t uiPortGetRunTimeCounterValue()
{
    uint64_t uxTimeInUsecs = uxPortGetTimeInUsec();

	/* note, there is no overflow protection for this 32b value in FreeRTOS
     *
     * This value will overflow in
     * ((0xFFFFFFFF)/(1000000*60)) minutes ~ 71 minutes
     *
     * We call LoadP_update() in idle loop (from vApplicationIdleHook) to accumlate the task load into a 64b value.
     * The implementation of LoadP_update() is in osal/src/freertos/LoadP_freertos.c
     * 
     */
    return (uint32_t)(uxTimeInUsecs);
}

/* Get the current time in microseconds. */
uint64_t uxPortGetTimeInUsec()
{
    uint64_t uxTimeInUsecs;
    uint32_t ulTimerCount;
    uint64_t uxTicks1;
    uint64_t uxTicks2;

    do {
        uxTicks1 = gTimerCntrl.uxTicks;
        ulTimerCount = TimerP_getCount(gTimerCntrl.pxTimerHandle);
        uxTicks2 = gTimerCntrl.uxTicks;
    } while (uxTicks1 != uxTicks2);

    /* Get the current time in microseconds */
    uxTimeInUsecs = uxTicks2 * (uint64_t)gTimerCntrl.ulUSecPerTick
                        + (uint64_t) ( /* convert timer count to usecs */
                            (uint64_t)(gTimerCntrl.ulTimerReloadCnt - ulTimerCount)*gTimerCntrl.ulUSecPerTick/(gTimerCntrl.ulTimerReloadCnt)
                            );

    return (uxTimeInUsecs);
}
/*-----------------------------------------------------------*/

void vPortTimerTickHandler()
{
    if ( ulPortSchedularRunning == pdTRUE )
    {
        /* Increment the RTOS tick. */
        if ( xTaskIncrementTick() != pdFALSE )
        {
            ullPortYieldRequired = pdTRUE;
        }
    }
}
/*-----------------------------------------------------------*/

void vPortTaskUsesFPU( void )
{
	/*
	 * A task is registering the fact that it needs an FPU context. Set the
	 * FPU flag (which is saved as part of the task context).
	 */
	ullPortTaskHasFPUContext = pdTRUE;

	/*
	 * Consider initialising the FPSR here - but probably not necessary in
	 * AArch64.
	 */
}
/*-----------------------------------------------------------*/

/*
 * This is used to make sure we are using the FreeRTOS API from within a valid interrupt priority level
 * In our A72 port this means IRQ.
 * i.e FreeRTOS API should not be called from FIQ, however right now we dont enforce it by checking
 * if we are in FIQ when this API is called.
 */
void vPortValidateInterruptPriority( void )
{
}
/*-----------------------------------------------------------*/

/*
 * configCHECK_FOR_STACK_OVERFLOW is set to 1, so the application must provide an
 * implementation of vApplicationStackOverflowHook()
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName )
{
    DebugP_log1("[FreeRTOS] Stack overflow detected for task [%s]", (uintptr_t)pcTaskName);
    DebugP_assert(0);
}
/*-----------------------------------------------------------*/

/*
 * configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task.
 */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/*
 * If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits.
 */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /*
	 * Pass out a pointer to the StaticTask_t structure in which the Idle task's
     * state will be stored.
     */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /*
	 * Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes.
     */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/*
 * configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 * application must provide an implementation of vApplicationGetTimerTaskMemory()
 * to provide the memory that is used by the Timer service task.
 */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
/*
 * If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits.
 */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /*
	 * Pass out a pointer to the StaticTask_t structure in which the Timer
     * task's state will be stored.
     */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /*
	 * Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configTIMER_TASK_STACK_DEPTH is specified in words, not bytes.
     */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/

/*
 * Returns true if the current core is in ISR context; low prio ISR, med prio ISR or timer tick ISR. High prio ISRs
 * aren't detected here, but they normally cannot call C code, so that should not be an issue anyway.
 */
BaseType_t xPortInIsrContext()
{
    BaseType_t inISR = false;
    if ( ullPortInterruptNesting != 0 )
    {
        inISR =  true;
    }
    return inISR;
}
/*-----------------------------------------------------------*/

/* This function is called when configUSE_IDLE_HOOK is 1 in FreeRTOSConfig.h */
__attribute__((weak)) void vApplicationIdleHook( void )
{
#if ( configLOAD_UPDATE_IN_IDLE == 1 )
/* Defined in osal/src/freertos/LoadP_freertos.c */
extern void vApplicationLoadHook();

    vApplicationLoadHook();
#endif

    asm ( " WFI " );
}
