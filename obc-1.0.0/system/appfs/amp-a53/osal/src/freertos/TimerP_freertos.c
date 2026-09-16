/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/src/nonos/timer/v1/TimerP_nonos.c */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* Freertos includes */
#include "FreeRTOS.h"

#include "osal/src/nonos/Nonos_config.h"
#include "csl/csl_timer.h"

/* Local Timer Struct */
typedef struct TimerP_Struct_s {
	bool     used;       /* In use or not status */
	uint32_t timerId;    /* timer Id */
	uint32_t periodType; /* Period type, default micro seconds */
//	uint32_t freqLo;     /* least siginificant 32-bits of frequency */
//	uint32_t freqHi;     /* most siginificant 32-bits of frequency  */
	uint32_t frequency;  /* timer clock frequency */ 
	uint32_t startMode;  /* timer start mode */
	uint32_t runMode;    /* timer run mode   */
	uint32_t period;     /* Period of a tick */
	TimerP_Fxn tickFxn;  /* Timer Tick function */
	void    *arg;        /* Argument passed into the timer function. */
	uint32_t availMask;   /* Available timer mask */
	HwiP_Handle hwi;      /* Hwi handle for tickFxn */
	uint32_t  intrNum;    /* Interrupt Number */
//	uint32_t rollovers;
//	uint32_t savedCurrCount;
//	uint32_t prevThreshold;
//	uint32_t tmar;
//	uint32_t tier;
//	uint32_t twer;
//	uint32_t tclr;
//	uint32_t tsicr;
//	uint32_t tiocpCfg;
} TimerP_Struct;

/*
 * As static allocation is used, the following provides the structs
 * The TimerP_Struct pool servers to get an available timer
 * handle during timer create API call.
 */
TimerP_Struct gTimerStructs[OSAL_NONOS_CONFIGNUM_TIMER] = {0U};
static uint32_t gTimerInitDone = 0U;
static uint32_t gTimerAnyMask;

/* external variables */
extern uint32_t gOsalTimerAllocCnt, gOsalTimerPeak;

/*
 * This private function returns the base address of the timer based on
 * the ID passed in.
 */
static uint32_t TimerP_getTimerBaseAddr(uint32_t timer_id)
{
	uint32_t addr;

	if (timer_id < TimerP_numTimerDevices) {
		addr = (uint32_t)gDmTimerPInfoTbl[timer_id].baseAddr;
	} else {
		addr = (uint32_t)0U;
	}
	return addr;
}

/*
 * This private function is a stub function for the timer ISR
 * function that is registered from the application
 */
static void TimerP_dmTimerStub(uintptr_t arg)
{
	TimerP_Struct *timer = (TimerP_Struct *)arg;
	uint32_t baseAddr = TimerP_getTimerBaseAddr(timer->timerId);
	uint32_t status;

	/* Disable the Timer interrupts */
	//TIMERIntDisable(baseAddr);

	/* Get the interrupt status */
	TIMERIntStatusGet(baseAddr, &status);

	/* Acknowledge the interrupt */
	TIMERIntStatusClear(baseAddr, status);

	/* Call the user's ISR */
	timer->tickFxn((uintptr_t)timer->arg);

	/* Enable the Timer interrupts */
	//TIMERIntEnable(baseAddr);
}

/*
 * This priviate function initializes the timer counter, period registers
 */
static void  TimerP_dmTimerInitDevice(TimerP_Struct *timer, uint32_t baseAddr)
{
	uintptr_t key = HwiP_disable();

	/* Disable the timer */
	TIMERDisable(baseAddr);

	/* 32-bit counter. The reset value is 0 (16-bit) */
	TIMERCounterSizeConfigure(baseAddr, TIMER_COUNTER_SIZE_32_BIT);

	/* Counter: 0 */
	TIMERCounterSet(baseAddr, 0u);

	/* Reload: 0 */
	TIMERReloadSet(baseAddr, 0u);

	HwiP_restore(key);
}

/*
 * This priviate function checks if there is any overflow in
 * timer period computations
 */
static bool TimerP_dmTimerCheckOverflow(uint32_t a, uint32_t b)
{
	return ((b > 0u) && (a > (TimerP_MAX_PERIOD / b)));
}

/*
 * This is a private function to set the period of the timer
 * register for a specified micro second value
 */
static bool TimerP_dmTimerSetMicroSeconds(TimerP_Struct *timer, uint32_t period)
{
	uint64_t counts;
	uint32_t prdCounts;
	uint32_t freqKHz;
	uint32_t roundUp;
	uint32_t baseAddr = TimerP_getTimerBaseAddr(timer->timerId);
	bool retVal = true;

	TimerP_stop(timer);

	roundUp = ((timer->frequency % 1000U) >= 500U) ? 1U : 0U;

	freqKHz = (timer->frequency / 1000U) + roundUp;
	if (TimerP_dmTimerCheckOverflow(freqKHz, period / 1000U)) {
		retVal = false;
	} else {
		counts = ((uint64_t)freqKHz * (uint64_t)period) / (uint64_t)1000U;
		if (counts > 0xffffffffU) {
			retVal = false;
		} else {
			/* AM654 timers count up from TLDR to 0xffffffff. */
			prdCounts = (uint32_t)counts;

			timer->period = prdCounts;
			timer->periodType = (uint32_t)TimerP_PeriodType_COUNTS;

			if ((timer->runMode == (uint32_t)TimerP_RunMode_CONTINUOUS) ||
				(timer->runMode == (uint32_t)TimerP_RunMode_ONESHOT)) {
				uint32_t load = 0U - prdCounts;
				TIMERCounterSet(baseAddr, load);
				TIMERReloadSet(baseAddr, load);
			} else {
				TIMERCounterSet(baseAddr, 0U);
			}
		}
	}
	return retVal;
}

/*
 * Thi is a private function to configure the timer registers for a given timer
 * period
 */
static TimerP_Status TimerP_dmTimerDeviceCfg(TimerP_Struct *timer, uint32_t baseAddr)
{
	uintptr_t key;
	TimerP_Status retVal = TimerP_OK;

    /* Initialize the timer */
	TimerP_dmTimerInitDevice(timer, baseAddr);

	key = HwiP_disable();

    /* Set the period */
	if (timer->periodType == (uint32_t)TimerP_PeriodType_MICROSECS) {
		if (!TimerP_dmTimerSetMicroSeconds(timer, timer->period)) {
			HwiP_restore(key);
			retVal = TimerP_FAILURE;
		}
	} else {
		if ((timer->runMode == (uint32_t)TimerP_RunMode_CONTINUOUS) ||
			(timer->runMode == (uint32_t)TimerP_RunMode_ONESHOT)) {
			uint32_t load = 0U - timer->period;
			TIMERCounterSet(baseAddr, load);
			TIMERReloadSet(baseAddr, load);
        } else {
			TIMERCounterSet(baseAddr, 0u);
		}
	}
   
	if (retVal != TimerP_FAILURE) {
//		HW_WR_REG32(baseAddr + TIMER_TCLR, timer->tclr);

		HwiP_restore(key);
	}

	return retVal;
}

/*
 * This is a private function to initialize the timer counter, period and compare
 * registers before starting the timers
 */
static TimerP_Status TimerP_dmTimerInitObj(TimerP_Struct *timer, TimerP_Fxn tickFxn, const TimerP_Params *params)
{
	timer->tickFxn = tickFxn;

	if (params->frequency != TimerP_USE_DEFAULT)
		timer->frequency = params->frequency;
	else
		timer->frequency = TimerP_getDefaultFreq(timer->timerId); /* Default, 3Mhz */

	if (params->period != 0u) {
		timer->period = params->period;
	} else {
		timer->period = 0u;
	}

	if (params->arg != NULL_PTR) {
		timer->arg = params->arg;
	}

	if (params->intrNum != TimerP_USE_DEFAULT) {
		timer->intrNum = params->intrNum;
	} else {
		timer->intrNum = gDmTimerPInfoTbl[timer->timerId].intrNum;
	}

	timer->periodType = params->periodType;
	timer->startMode  = params->startMode;
	timer->runMode    = params->runMode;

	return TimerP_OK;
}

/*
 * This is a private function to initialize the timer instance, that sets up the timer ISR to
 * a specified timer id and sets up the timer compare, counter and period registers
 */
static TimerP_Status TimerP_dmTimerInstanceInit(TimerP_Struct *timer, uint32_t id, TimerP_Fxn tickFxn, const TimerP_Params *params)
{
	TimerP_Status ret = TimerP_OK;
	uintptr_t     key;
	uint32_t      i;
	uint32_t      tempId = 0xffffu;
	OsalRegisterIntrParams_t intrRegParams;
	uint32_t      baseAddr;
	int32_t       intrNum;
	uint32_t      timerPAnyMask = (uint32_t)(TimerP_ANY_MASK);

	if ((id != TimerP_ANY) && (id >= TimerP_numTimerDevices)) {
		ret = TimerP_FAILURE;
	}

	/* Get the timer Id */
	if (ret == TimerP_OK) {
		/* Set the available timer id mask to all */
		if (gTimerInitDone == 0U) {
			gTimerAnyMask  = TimerP_AVAILABLE_MASK;
			gTimerInitDone = 1U;
		}

		timer->availMask = gTimerAnyMask;

		key = HwiP_disable();

		if (id == TimerP_ANY) {
			for (i = 0u; i < TimerP_numTimerDevices; i++) {
				uint32_t shift, nshift;
				shift  = ((uint32_t)1u) << i;
				nshift = ~shift;
				if (((timerPAnyMask & shift) != (uint32_t)0u) &&
					((timer->availMask & shift) != (uint32_t)0u)) {
					timer->availMask &= nshift;
					tempId = i;
					break;
				}
			}
		} else {
			if ((timer->availMask & ((uint32_t)1u << id)) > 0u) {
				uint32_t shift, nshift;
				shift  = ((uint32_t) 1u) << id;
				nshift = ~shift;
				timer->availMask &= nshift;
				tempId = id;
			}
		}

		gTimerAnyMask = timer->availMask;

		/* Initialize the timer state object */
		timer->timerId = tempId; /* Record the timer Id */
		baseAddr = TimerP_getTimerBaseAddr(timer->timerId);

		HwiP_restore(key);

		if ((tempId == 0xffffU) || (0x0U == baseAddr)) {
			ret = TimerP_NOT_AVAILABLE;
		}
	}

	/* Initialize the timer state object */
	if (ret == TimerP_OK) {
		ret = TimerP_dmTimerInitObj(timer, tickFxn, params);
	}

	/* Create the Hwi Funtion for the tick funtion */
	if (ret == TimerP_OK) {
		if (timer->tickFxn != (TimerP_Fxn)NULL_PTR) {
			intrNum = timer->intrNum;
			/* Initialize with defaults */
			Osal_RegisterInterrupt_initParams(&intrRegParams);

			/* Populate the interrupt parameters */
			intrRegParams.corepacConfig.arg = (uintptr_t)timer;
			intrRegParams.corepacConfig.name = (char *)NULL_PTR;
			intrRegParams.corepacConfig.isrRoutine = TimerP_dmTimerStub;

			intrRegParams.corepacConfig.intrType = (uint32_t)OSAL_GIC_INTR_TYPE_HIGH_LEVEL;
			intrRegParams.corepacConfig.intrPrio = 0x20U;

			intrRegParams.corepacConfig.intrNum = intrNum; /* Host Interrupt Number */

			/* Register interrupts */
			Osal_RegisterInterrupt(&intrRegParams, &(timer->hwi));

			if (timer->hwi == NULL_PTR) {
				ret = TimerP_ISR_HOOK_ERR;
			}
		} else {
			timer->hwi = NULL_PTR;
		}
  	}

	/* timer post init */
	if (ret == TimerP_OK) {
		ret = TimerP_dmTimerDeviceCfg(timer, baseAddr);

		if (timer->startMode == (uint32_t)TimerP_StartMode_AUTO) {
			TimerP_start(timer);
		}
	}

	return ret;
}

/* Interface functions */

/* Default parameter initialization module function
 * more details on the documentation of this interface
 * function can be found in the TimerP.h interface
 * header file */
void TimerP_Params_init(TimerP_Params *params)
{
	OSAL_Assert((params == NULL_PTR));

	if (params != NULL_PTR) {
		/* Set the default values */
		params->arg         = NULL_PTR;
//		params->extfreqHi   = TimerP_USE_DEFAULT;
//		params->extfreqLo   = TimerP_USE_DEFAULT;
//		params->intfreqHi   = TimerP_USE_DEFAULT;
//		params->intfreqLo   = TimerP_USE_DEFAULT;
		params->frequency   = TimerP_USE_DEFAULT;
		params->name        = (char *)NULL_PTR;
		params->period      = 0;
		params->runMode     = TimerP_RunMode_CONTINUOUS;
		params->startMode   = TimerP_StartMode_AUTO;
		params->periodType  = TimerP_PeriodType_MICROSECS;
		params->intrNum      = TimerP_USE_DEFAULT;
	}
}

/* Timer create module function
 * more details on the documentation of this interface
 * function can be found in the TimerP.h interface
 * header file */
TimerP_Handle TimerP_create(int32_t id,
							TimerP_Fxn tickFxn,
							const TimerP_Params *inParams)

{
	uint32_t          i;
	uintptr_t         key;
	TimerP_Handle     ret_handle;
	TimerP_Params     params;

	if (inParams != NULL_PTR) {
		params = *inParams;
	} else {
		TimerP_Params_init(&params);
	}

	key = HwiP_disable();

	for (i = 0; i < OSAL_NONOS_CONFIGNUM_TIMER; i++) {
		if (gTimerStructs[i].used == false) {
			gTimerStructs[i].used = true;

			/* Update statistics */
			gOsalTimerAllocCnt++;
			if (gOsalTimerAllocCnt > gOsalTimerPeak) {
				gOsalTimerPeak = gOsalTimerAllocCnt;
			}

			gTimerStructs[i].timerId = (uint32_t)id;

			break;
		}
	}
	HwiP_restore(key);

	if (i == OSAL_NONOS_CONFIGNUM_TIMER) {
		ret_handle = NULL_PTR;
	} else {
		/* Start the timer for that period/mode */
		ret_handle = ((TimerP_Handle)&gTimerStructs[i]);
	}

	if (ret_handle != NULL_PTR) {
		if (TimerP_dmTimerInstanceInit(&gTimerStructs[i], (uint32_t)id, tickFxn, &params) != TimerP_OK) {
			/* Free up the consumed timer memory */
			gTimerStructs[i].used = false;
			ret_handle = NULL_PTR;
		}
	}

	/* Start timer if it is auto mode */
	if ((params.startMode == (uint32_t)TimerP_StartMode_AUTO) &&
		(ret_handle != NULL_PTR) ) {
		if (TimerP_start((TimerP_Handle)&gTimerStructs[i]) != TimerP_OK) {
			gTimerStructs[i].used = false;
			ret_handle = NULL_PTR;
		}
	}
	return ret_handle;
}

/* Timer delete module function
 * more details on the documentation of this interface
 * function can be found in the TimerP.h interface
 * header file */
TimerP_Status TimerP_delete(TimerP_Handle handle)
{
	/* Release or free the memory */
	uintptr_t key;
	uint32_t id;
	uint32_t index = (uint32_t)((((uintptr_t)handle) - ((uintptr_t)gTimerStructs) ) / sizeof(TimerP_Struct)); /* struct subtraction */
	TimerP_Status ret = TimerP_OK;
	TimerP_Struct *timer = (TimerP_Struct *)handle;

	OSAL_Assert((handle == NULL_PTR));

	key = HwiP_disable();

	id = gTimerStructs[index].timerId;

	if ((timer != NULL_PTR) && ((gTimerStructs[index].used) == true)) {
		uint32_t shift = ((uint32_t)1u) << id;

		/* Clear the ISR that was set before */
		if (timer->hwi != NULL_PTR) {
			HwiP_delete(timer->hwi);
		}

		/* Reset the timer's bit field in the mask and clear the used flag */
		gTimerStructs[index].used = false;
		if (((gTimerAnyMask & shift)) == 0U) {
			gTimerAnyMask |= shift;
		} else {
			ret = TimerP_FAILURE;	
		}

		/* Found the osal timer object to delete */
		if (gOsalTimerAllocCnt > 0U) {
			gOsalTimerAllocCnt--;
		}
	} else {
		ret = TimerP_FAILURE;	
	}

	HwiP_restore(key);

	return ret;
}

/* Timer start module function
 * more details on the documentation of this interface
 * function can be found in the TimerP.h interface
 * header file */
TimerP_Status TimerP_start(TimerP_Handle handle)
{
	uintptr_t key;
	uint32_t runMode;
	TimerP_Struct *timer = (TimerP_Struct *)handle;
	TimerP_Status retVal = TimerP_OK;
	uint32_t baseAddr = TimerP_getTimerBaseAddr(timer->timerId);

	if (baseAddr == 0U) {
		retVal = TimerP_NOT_AVAILABLE;
	} else {
		key = HwiP_disable();

		TimerP_stop(handle);

		if ((timer->runMode == (uint32_t)TimerP_RunMode_CONTINUOUS) ||
			(timer->runMode == (uint32_t)TimerP_RunMode_ONESHOT)) {
			TIMERCounterSet(baseAddr, 0U - timer->period); /* Set AM654 preload */
		} else {
			TIMERCounterSet(baseAddr, 0u); /* Set timer count back to initial value */
		}

		if (timer->hwi != NULL_PTR) {
			Osal_ClearInterrupt(0 /* Not used */, timer->intrNum);
			Osal_EnableInterrupt(0 /* Not used */, timer->intrNum);
		} else {
			retVal = TimerP_FAILURE;
		}

		if (timer->runMode == (uint32_t)TimerP_RunMode_CONTINUOUS) {
			runMode = TIMER_RELOAD_MODE; /* Period */
		} else if (timer->runMode == (uint32_t)TimerP_RunMode_ONESHOT) {
			runMode = TIMER_ONESHOT_MODE;
		} else {
			runMode = TIMER_FREERUN_MODE; /* Not used */
		}

		if (retVal == TimerP_OK) {
			TIMERModeConfigure(baseAddr, runMode);
			TIMERIntEnable(baseAddr);

			TIMEREnable(baseAddr);
		}
		HwiP_restore(key);
	}

	return retVal;
}

/* Timer stop module function
 * more details on the documentation of this interface
 * function can be found in the TimerP.h interface
 * header file */
TimerP_Status TimerP_stop(TimerP_Handle handle)
{
	uint32_t status;
	TimerP_Struct *timer = (TimerP_Struct *)handle;
	TimerP_Status retVal = TimerP_OK;
	uint32_t baseAddr = TimerP_getTimerBaseAddr(timer->timerId);

	if (baseAddr == 0U) {
		retVal = TimerP_NOT_AVAILABLE;
	} else {
		TIMERDisable(baseAddr);

		TIMERIntStatusGet(baseAddr, &status);

		if (status > 0U) {
			/* Clear all pending interrupts */
			TIMERIntStatusClear(baseAddr, status);
		}

		if (timer->hwi != NULL_PTR) {
			Osal_ClearInterrupt(0 /* Not used */, timer->intrNum);
			Osal_DisableInterrupt(0 /* Not used */, timer->intrNum);
		} else {
			retVal = TimerP_FAILURE;
		}
	}

	return retVal;
}

/* Timer set micro seconds module function
 * more details on the documentation of this interface
 * function can be found in the TimerP.h interface
 * header file */
TimerP_Status TimerP_setPeriodMicroSecs(TimerP_Handle handle, uint32_t microsecs)
{
	TimerP_Struct *timer = (TimerP_Struct *)handle;
	TimerP_Status retVal = TimerP_OK;

	if (!TimerP_dmTimerSetMicroSeconds(timer, microsecs)) {
		retVal = TimerP_FAILURE;
	}

	return retVal;
}

/* Get time in micro seconds */
uint64_t TimerP_getTimeInUsecs(void)
{
	uint64_t curTime;
	/*
	 * In case of FreeRTOS get the current counter value from
	 * the timer used by OS itself, instead of using an 
	 * additional timer for the same.
	 */
	curTime = uiPortGetRunTimeCounterValue();

	return curTime;
}

/* Get timer reload count */
uint32_t TimerP_getReloadCount(TimerP_Handle handle)
{
	TimerP_Struct *timer = (TimerP_Struct *)handle;
	return timer->period;
}

/* Get timer current count */
uint32_t TimerP_getCount(TimerP_Handle handle)
{
	uint32_t count = 0U;
	uint32_t load = 0U;
	TimerP_Struct *timer = (TimerP_Struct *)handle;
	uint32_t baseAddr = TimerP_getTimerBaseAddr(timer->timerId);

	if (baseAddr != 0U) {
		TIMERCounterGet(baseAddr, &count);
		TIMERReloadGet(baseAddr, &load);
		/* Convert the up-counter value to elapsed counts since TLDR. */
		count -= load;
	}

	return count;
}
