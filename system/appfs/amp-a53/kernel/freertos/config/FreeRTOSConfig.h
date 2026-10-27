/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _FREERTOS_CONFIG_H_
#define _FREERTOS_CONFIG_H_

#include "osal/DebugP.h" /* for DebugP_assert() */

#if defined(__BUILD_A53_CORE0__)
#include "FreeRTOSConfig_A53_Core0.h"
#elif defined(__BUILD_A53_CORE1__)
#include "FreeRTOSConfig_A53_Core1.h"
#elif defined(__BUILD_A53_CORE2__)
#include "FreeRTOSConfig_A53_Core2.h"
#elif defined(__BUILD_A53_CORE3__)
#include "FreeRTOSConfig_A53_Core3.h"
#elif defined(__BUILD_A53_CORE4__)
#include "FreeRTOSConfig_A53_Core4.h"
#elif defined(__BUILD_A53_CORE5__)
#include "FreeRTOSConfig_A53_Core5.h"
#else
#error "Unknown build option, review the documentation for the specific options to ensure they are compatible!"
#endif

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE AND IN THE
 * FreeRTOS REFERENCE MANUAL.
 *----------------------------------------------------------*/

/* Keep below as 1 if the most optmized task switching latency is needed.
 * This disables tracing, logging, assert and other error checks.
 * So unless every last cycle of task switching is important leave this as 0.
 * 
 * This is not a FreeRTOS defined config and is defined by TI to quickly switch
 * between optimized and not-so-optimized config
 */
#define configOPTIMIZE_FOR_LATENCY              (0)

#define configUSE_PREEMPTION					(1)
#define configUSE_PORT_OPTIMISED_TASK_SELECTION	(1)
#define configUSE_TICKLESS_IDLE                 (0)
#define configUSE_IDLE_HOOK                     (0)
#define configUSE_MALLOC_FAILED_HOOK            (0)
#define configUSE_DAEMON_TASK_STARTUP_HOOK      (0)
#define configUSE_TICK_HOOK                     (0)
/* The case configSYSTICK_CLOCK_HZ is not needed and should be left undefined */
//#define configCPU_CLOCK_HZ                      (0)
#define configSYSTICK_CLOCK_HZ                  (3000000)	/* See https://www.freertos.org/a00110.html */
#define configTICK_RATE_HZ                      (1000)
#define configMAX_PRIORITIES                    (16)
#define configMINIMAL_STACK_SIZE                (1024) /* in units of configSTACK_DEPTH_TYPE, not bytes */
#define configMAX_TASK_NAME_LEN                 (32)
#define configUSE_TRACE_FACILITY                (1)
#define configUSE_STATS_FORMATTING_FUNCTIONS    (0)
#define configUSE_16_BIT_TICKS                  (0)
#define configIDLE_SHOULD_YIELD                 (1)
#define configUSE_TASK_NOTIFICATIONS            (1)
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   (1)
#define configUSE_MUTEXES                       (1)
#define configUSE_RECURSIVE_MUTEXES             (1)
#define configUSE_COUNTING_SEMAPHORES           (1)
#define configUSE_ALTERNATIVE_API               (0)

/* when = 1, Need to provied below,
 *    void vApplicationStackOverflowHook( TaskHandle_t xTask,
 *           char *pcTaskName ); 
 */
#if (configOPTIMIZE_FOR_LATENCY==0)
#define configCHECK_FOR_STACK_OVERFLOW          (2) 
#else
#define configCHECK_FOR_STACK_OVERFLOW          (0) 
#endif
#define configRECORD_STACK_HIGH_ADDRESS         (1)
#define configQUEUE_REGISTRY_SIZE               (32)
#define configUSE_QUEUE_SETS                    (0)
#define configUSE_TIME_SLICING                  (0) /* keep as 0 to get same functionality as SysBIOS6 */
#define configUSE_NEWLIB_REENTRANT              (0)
#define configENABLE_BACKWARD_COMPATIBILITY     (0)
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS (4)
#define configSTACK_DEPTH_TYPE                  UBaseType_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configSUPPORT_STATIC_ALLOCATION         (1) /* when = 1, need to provide below,
                                                     *
                                                     * void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                                     *      StackType_t **ppxTimerTaskStackBuffer,
                                                     *      uint32_t *pulTimerTaskStackSize );
                                                     * 
                                                     * void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                                     *      StackType_t **ppxIdleTaskStackBuffer,
                                                     *      uint32_t *pulIdleTaskStackSize );
                                                     */ 
#define configSUPPORT_DYNAMIC_ALLOCATION        (1)
#define configTOTAL_HEAP_SIZE                   (3 * 1024 * 1024) /* not used when heap_3.c is the selected heap */
#define configAPPLICATION_ALLOCATED_HEAP        (0)    

/* run-time stats config */
#if (configOPTIMIZE_FOR_LATENCY==0)
#define configGENERATE_RUN_TIME_STATS           (1)
#else
#define configGENERATE_RUN_TIME_STATS           (0)
#endif
void vPortConfigTimerForRunTimeStats();
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vPortConfigTimerForRunTimeStats()
uint32_t uiPortGetRunTimeCounterValue();
#define portGET_RUN_TIME_COUNTER_VALUE()        uiPortGetRunTimeCounterValue()

/* co-routine related config */
#define configUSE_CO_ROUTINES                   (0)
#define configMAX_CO_ROUTINE_PRIORITIES         (0)

/* timer related config */
#define configUSE_TIMERS                        (1)
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                (16)
#define configTIMER_TASK_STACK_DEPTH            (256)

#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (0xE0U)
#define configKERNEL_INTERRUPT_PRIORITY         (configMAX_SYSCALL_INTERRUPT_PRIORITY)
#define configMAX_API_CALL_INTERRUPT_PRIORITY   (configMAX_SYSCALL_INTERRUPT_PRIORITY)

#if (configOPTIMIZE_FOR_LATENCY==0)
#define configASSERT(x)                        DebugP_assert( (uintptr_t)(x))
#endif

/* MPU aware FreeRTOS */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS  (0)
#define configTOTAL_MPU_REGIONS                                 (0)
#undef  configTEX_S_C_B_FLASH
#undef  configTEX_S_C_B_SRAM
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY             (0)

/* defines need for FreeRTOS+POSIX*/
#define configUSE_POSIX_ERRNO           (1)
#define configUSE_APPLICATION_TASK_TAG  (1)

/* include specific functions */
#define INCLUDE_vTaskDelete         (1)
#define INCLUDE_vTaskDelay          (1)
#define INCLUDE_vTaskSuspend        (1)
#define INCLUDE_xTimerDelete        (1)
#define INCLUDE_vSemaphoreDelete    (1)
#define INCLUDE_xEventGroupSetBitFromISR (1)
#define INCLUDE_xTimerPendFunctionCall   (1)
#define INCLUDE_xTaskGetIdleTaskHandle   (1)
#define INCLUDE_vTaskPrioritySet         (1)
#define INCLUDE_xSemaphoreGetMutexHolder (1)
/* 
 * This is not a FreeRTOS defined config and is defined to enable
 * load update in idle task 
 */
#define configLOAD_UPDATE_IN_IDLE   (1)
#define configLOAD_WINDOW_IN_MS     (500)

#if (configLOAD_UPDATE_IN_IDLE==1)
#undef  configUSE_IDLE_HOOK
#define configUSE_IDLE_HOOK         (1)
#endif

/*
 * This is not a FreeRTOS defined config and is defined to create tasks with
 * a floating point context.
 *
 * When = 2, means that each task has an FPU context already when initializing
 * task stack. so there is no need to call vPortTaskUsesFPU() before any floating 
 * point instructions are executed
 */
#define configUSE_TASK_FPU_SUPPORT	(2)

#endif /* _FREERTOS_CONFIG_H_ */
