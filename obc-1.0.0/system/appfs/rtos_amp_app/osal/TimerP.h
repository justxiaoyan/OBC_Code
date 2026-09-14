/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/TimerP.h */

#ifndef _OSAL_TIMERP_H_
#define _OSAL_TIMERP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Const used to specify any timer
 */
#define TimerP_ANY       (~(uint32_t)0u)

/*!
 * Max value of Timer period for PeriodType_COUNTS
 */
#define TimerP_MAX_PERIOD ((uint32_t)0xffffffffu)

/*!
 * Use default values as provided by the OSAL package
 */
#define TimerP_USE_DEFAULT   (-(int32_t)1)

/*!
 *  @brief  Frequency-in-hertz struct
 */
typedef struct TimerP_FreqHz_s {
	uint32_t hi;      /*!< most significant 32-bits of frequency */
	uint32_t lo;      /*!< least significant 32-bits of frequency */
} TimerP_FreqHz;

/*!
 *  @brief    Status codes for TimerP APIs
 */
typedef enum TimerP_Status_e {
	TimerP_OK            =             0,
	TimerP_FAILURE       = (-(int32_t) 1),
	TimerP_NOT_AVAILABLE = (-(int32_t) 2),
	TimerP_BAD_INT_NUM   = (-(int32_t) 3),
	TimerP_ISR_HOOK_ERR  = (-(int32_t) 4)
} TimerP_Status;

/*!
 *  @brief  Timer period units for TimerP APIs
 */
typedef enum TimerP_PeriodType_e {
	TimerP_PeriodType_MICROSECS = ( (uint32_t) 0), /*!< timer period is in micro seconds */
	TimerP_PeriodType_COUNTS    = ( (uint32_t) 1), /*!< timer period is in counts */
} TimerP_PeriodType;

/*!
 *  @brief  Timer runtime modes for TimerP APIs
 */
typedef enum TimerP_RunMode_e {
	TimerP_RunMode_CONTINUOUS = ( (uint32_t) 0), /*!< timer is periodic and runs continuously */
 	TimerP_RunMode_ONESHOT    = ( (uint32_t) 1), /*!< timer runs for a single period values and stops */
 } TimerP_RunMode;


/*!
 *  @brief  Timer start modes for TimerP APIs
 */
typedef enum TimerP_StartMode_e {
	TimerP_StartMode_AUTO =  ( (uint32_t) 0),     /*!< timer starts automatically after create*/
	TimerP_StartMode_USER =  ( (uint32_t) 1),     /*!< timer will be started by the user */
} TimerP_StartMode;

/*!
 *  @brief    Opaque client reference to an instance of a TimerP
 *
 *  A TimerP_Handle returned from the ::TimerP_create represents that instance.
 *  and then is used in the other instance based functions (e.g. ::TimerP_start,
 *  ::TimerP_stop, etc.).
 */
typedef  void *TimerP_Handle;

/*!
 *  @brief  Prototype for a TimerP function.
 */
typedef void (*TimerP_Fxn)(uintptr_t arg);

/*!
 *  @brief    Basic TimerP Parameters
 *
 *  Structure that contains the parameters passed into ::TimerP_create
 *  when creating a TimerP instance. The ::TimerP_Params_init function should
 *  be used to initialize the fields to default values before the application sets
 *  the fields manually. The TimerP default parameters are noted in
 *  TimerP_Params_init.
 */
typedef struct TimerP_Params_s {
	char    *name;        /*!< Name of the timer instance. Memory must
                             persist for the life of the clock instance.
                             This can be used for debugging purposes, or
                             set to NULL if not needed. */
	uint32_t periodType;  /*!< Period type, default micro seconds */
//	int32_t  extfreqLo;   /*!< least siginificant 32-bits of ext frequency
//                               set to 0 to use internal clk freq  */
//	int32_t  extfreqHi;   /*!< most siginificant 32-bits of ext frequency
//                               set to 0 to use internal clk freq  */
//	int32_t  intfreqLo;   /*!< least siginificant 32-bits of int frequency
//                               set to 0 to use default internal clk freq  */
//	int32_t  intfreqHi;   /*!< most siginificant 32-bits of int frequency
//                               set to 0 to use default internal clk freq  */
	int32_t frequency;   /*!< timer clock frequency */
	uint32_t startMode;   /*!< timer start mode */
	uint32_t runMode;     /*!< timer run mode */
	uint32_t period;      /*!< Period of a tick */
//	TimerP_Timer64Mode timerMode; /*!< timer mode for 64bit timer */
//	TimerP_Timer64Half timerHalf; /*!< timer half for 64bit timer */

	int32_t  intrNum;      /*!< Hwi Interrupt number to be used by Timer */
 	void*    arg;         /*!< Argument passed into the timer function. */
} TimerP_Params;

/*!
 *  @brief  Function to create a timer object.
 *
 *  @param  id       Timer Id
 *
 *  @param  tickFxn  Function that runs upon timer expiry
 *
 *  @param  timerParams    Pointer to the instance configuration parameters. NULL
 *                    denotes to use the default parameters. The TimerP default
 *                    parameters are noted in ::TimerP_Params_init.
 *
 *  @return A TimerP_Handle on success or a NULL on an error.  This handle can
 *          be passed to TimerP_start()
 */
extern TimerP_Handle TimerP_create(int32_t        id,
								   TimerP_Fxn     tickFxn,
								   const TimerP_Params *timerParams);

/*!
 *  @brief  Function to delete a timer.
 *
 *  @param  timerHandle  A TimerP_Handle returned from ::TimerP_create
 *
 *  @return Status of the function.
 *    - TimerP_OK: Deleted the timer instance
 *    - TimerP_FAILURE: Timed out waiting to delete the timer object.
 */
extern TimerP_Status TimerP_delete(TimerP_Handle timerHandle);

/*!
 *  @brief  Function to set timer period specified in micro seconds
 *  A best-effort method will be used to set the period register.
 *  There might be a slight rounding error based on resolution of
 *  timer period register. If the timer frequency cannot support
 *  the requested period, i.e. the timer period register cannot
 *  support the requested period, then this function returns false.
 *  TimerP_setPeriodMicroSecs() invokes TimerP_stop() prior to setting
 *  the period and leaves the timer in the stopped state.
 *
 *  @param  timerHandle  A TimerP_Handle returned from ::TimerP_create
 *
 *  @param  microsecs  time in micro seconds
 *
 *  @return Status of the function.
 *    - TimerP_OK: Deleted the timer instance
 *    - TimerP_FAILURE: Timed out waiting to delete the timer object.
 */
extern TimerP_Status TimerP_setPeriodMicroSecs(TimerP_Handle timerHandle, uint32_t microsecs);

/*!
 *  @brief  Initialize params structure to default values.
 *
 *  The default parameters are:
 *   - name: NULL
 *   - arg: 0
 *
 *  @param timerParams  Pointer to the instance configuration parameters.
 */
extern void TimerP_Params_init(TimerP_Params *timerParams);

/*!
 *  @brief  Function to start a timer.
 *
 *  @param  timerHandle  A TimerP_Handle returned from ::TimerP_create
 *
 *  @return Status of the functions
 *    - TimerP_OK: Scheduled the timer function successfully
 *    - TimerP_FAILURE: The API failed.
 */
extern TimerP_Status TimerP_start(TimerP_Handle timerHandle);

/*!
 *  @brief  Function to stop a timer.
 *
 *  @param  timerHandle  A TimerP_Handle returned from ::TimerP_create
 *
 *  @return Status of the functions
 *    - TimerP_OK: Scheduled the timer function successfully
 *    - TimerP_FAILURE: The API failed.
 */
extern TimerP_Status TimerP_stop(TimerP_Handle timerHandle);

/*!
 *  @brief  Function to clear the interrupt of the timer.
 *
 *  @param  timerHandle  A TimerP_Handle returned from ::TimerP_create
 *
 *  @return Status of the function
 *    - TimerP_OK: Clear the interrupt of the timer function successfully.
 *
 */
extern TimerP_Status TimerP_ClearInterrupt(TimerP_Handle timerHandle);

/*!
 *  @brief  Function to return current time in units of micro-secs
 *
 *  @return Current time in units of micro-secs
 *
 * Note: This API is not supported for J7/OMAPL13x platforms and
 *       result in link errors if used in the application
 */
extern uint64_t TimerP_getTimeInUsecs(void);

/**
 *  @brief Get timer reload count
 *
 *  @param  timerHandle  A TimerP_Handle returned from ::TimerP_create
 *
 *  @return Reload count value
 */
extern uint32_t TimerP_getReloadCount(TimerP_Handle timerHandle);

/**
 *  @brief Get timer current count
 *
 *  @param  timerHandle  A TimerP_Handle returned from ::TimerP_create
 *
 *  @return Current timer count value
 */
uint32_t TimerP_getCount(TimerP_Handle timerHandle);

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_TIMERP_H_ */
