/*
 * Copyright (C) 2021-2022 Hikvision Auto Technology Co., Ltd.
 *   Author: wsl <wangshulin@hikvision.com>
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
	extern "C" {
#endif

#include "osal/HwiP.h" /* for HwiP_disable(), HwiP_restore() */

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the given hardware
 * and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	size_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;
typedef portBASE_TYPE BaseType_t;
typedef uint64_t UBaseType_t;

typedef uint64_t TickType_t;
#define portMAX_DELAY ( ( TickType_t ) 0xffffffffffffffff )


/*-----------------------------------------------------------*/

/* Hardware specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			16
#define portPOINTER_SIZE_TYPE 		uint64_t

/*-----------------------------------------------------------*/

/* Task utilities. */

/* Called at the end of an ISR that can cause a context switch. */
#define portEND_SWITCHING_ISR( xSwitchRequired )\
{												\
extern uint64_t ullPortYieldRequired;			\
												\
	if ( xSwitchRequired != pdFALSE )			\
	{											\
		ullPortYieldRequired = pdTRUE;			\
	}											\
}

#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )
#define portYIELD() __asm volatile ( "SVC 0" ::: "memory" )
/*-----------------------------------------------------------
 * Critical section control
 *----------------------------------------------------------*/

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portDISABLE_INTERRUPTS()									\
	__asm volatile ( "MSR DAIFSET, #3" ::: "memory" );				\
	__asm volatile ( "DSB SY" );									\
	__asm volatile ( "ISB SY" );

#define portENABLE_INTERRUPTS()										\
	__asm volatile ( "MSR DAIFCLR, #3" ::: "memory" );				\
	__asm volatile ( "DSB SY" );									\
	__asm volatile ( "ISB SY" );


/*
 * These macros do not globally disable/enable interrupts.  They do mask off
 * interrupts that have a priority below configMAX_API_CALL_INTERRUPT_PRIORITY.
 */
#define portENTER_CRITICAL()		vPortEnterCritical();
#define portEXIT_CRITICAL()			vPortExitCritical();
#define portSET_INTERRUPT_MASK_FROM_ISR()		HwiP_disable()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	HwiP_restore( x )

/*-----------------------------------------------------------*/

/*
 * Task function macros as described on the FreeRTOS.org WEB site.  These are
 * not required for this port but included in case common demo code that uses these
 * macros is used.
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )	void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )	void vFunction( void *pvParameters )

/*
 * Any task that uses the floating point unit MUST call vPortTaskUsesFPU()
 * before any floating point instructions are executed.
 */
void vPortTaskUsesFPU( void );
#define portTASK_USES_FLOATING_POINT() vPortTaskUsesFPU()

/* Architecture specific optimisations. */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

	/* Store/clear the ready priorities in a bit map. */
	#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
	#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

	/*-----------------------------------------------------------*/

	#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - __builtin_clz( uxReadyPriorities ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

#ifdef configASSERT
	void vPortValidateInterruptPriority( void );
	#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 	vPortValidateInterruptPriority()
#endif /* configASSERT */

#define portNOP() __asm volatile( "NOP" )
#define portINLINE __inline

extern BaseType_t xPortInIsrContext(void);

#ifdef __cplusplus
	} /* extern C */
#endif

#define portMEMORY_BARRIER() __asm volatile( "" ::: "memory" )

#endif /* PORTMACRO_H */
