/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

/**
 *  \file  timer.c
 *
 *  \brief Timer APIs.
 *
 *   This file contains the device abstraction layer APIs for Timer.
 */

#include <stdint.h>
#include <stddef.h>
#include <errno.h>	/* for EINVAL */

#include "csl/cslr_timer.h"
#include "csl/src/ip/timer/V0/timer.h"
#include "csl/hw_types.h"

int32_t TIMEREnable(uint32_t baseAddr)
{
	int32_t retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		/* Enable the timer */
		HW_WR_FIELD32(baseAddr + TIMER_CONTROL,
				TIMER_CONTROL_TIMER_ENABLE,
				TIMER_CONTROL_TIMER_ENABLE_VALUE_1);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERDisable(uint32_t baseAddr)
{
	int32_t retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		/* Disable the timer */
		HW_WR_FIELD32(baseAddr + TIMER_CONTROL,
				TIMER_CONTROL_TIMER_ENABLE,
				TIMER_CONTROL_TIMER_ENABLE_VALUE_0);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERModeConfigure(uint32_t baseAddr, uint32_t timerMode)
{
	int32_t  retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		if ((timerMode == TIMER_ONESHOT_MODE) ||
			(timerMode == TIMER_RELOAD_MODE) ||
			(timerMode == TIMER_FREERUN_MODE)) {
			uint32_t regVal = HW_RD_REG32(baseAddr + TIMER_CONTROL);
			/* AM654 TCLR.AR selects auto-reload; zero selects one-shot/free-run. */
			regVal &= ~TIMER_CONTROL_TIMER_MODE_MASK;
			if (timerMode == TIMER_RELOAD_MODE)
				regVal |= TIMER_CONTROL_TIMER_MODE_MASK;
			HW_WR_REG32(baseAddr + TIMER_CONTROL, regVal);
			retVal = 0;
		}
	}

	return retVal;
}

int32_t TIMERCounterSizeConfigure(uint32_t baseAddr, uint32_t counterSize)
{
	int32_t  retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		if (counterSize == TIMER_COUNTER_SIZE_32_BIT) {
			/* AM654 DMTimer V2 is a fixed 32-bit counter. */
			retVal = 0;
		}
	}

	return retVal;
}

int32_t TIMERPreScalerClkEnable(uint32_t baseAddr, uint32_t preClkTimeValue)
{
	int32_t  retVal = -EINVAL;

	if ((baseAddr != (uint32_t)(0u)) &&
		((preClkTimeValue & ~TIMER_CONTROL_TIMER_PRESCALER_MASK) == 0U)) {
		uint32_t regVal = HW_RD_REG32(baseAddr + TIMER_CONTROL);
		regVal &= ~TIMER_CONTROL_TIMER_PRESCALER_MASK;
		regVal |= preClkTimeValue;
		HW_WR_REG32(baseAddr + TIMER_CONTROL, regVal);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERPreScalerClkDisable(uint32_t baseAddr)
{
	int32_t retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		/* Disable Pre-scaler clock */
		HW_WR_FIELD32(baseAddr + TIMER_CONTROL,
				TIMER_CONTROL_TIMER_PRESCALER,
				TIMER_CONTROL_TIMER_PRESCALER_VALUE_0);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERCounterSet(uint32_t baseAddr, uint32_t counter)
{
    int32_t retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		/* TCRR holds the live counter; TLDR is the reload value. */
		HW_WR_REG32(baseAddr + TIMER_VALUE, counter);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERCounterGet(uint32_t baseAddr, uint32_t *pCounterVal)
{
	int32_t retVal = -EINVAL;

	if ((baseAddr != (uint32_t)(0u)) && (pCounterVal != NULL)) {
		/* Get the current counter value (internal),  see also the TIMER_LOAD register. */
		*pCounterVal = HW_RD_REG32(baseAddr + TIMER_VALUE);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERReloadSet(uint32_t baseAddr, uint32_t reload)
{
	int32_t retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		/* Load the register with the re-load value */
		HW_WR_REG32(baseAddr + TIMER_RELOAD, reload);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERReloadGet(uint32_t baseAddr, uint32_t *pReloadVal)
{
	int32_t retVal = -EINVAL;

	if ((baseAddr != (uint32_t)(0u)) && (pReloadVal != NULL)) {
		*pReloadVal = HW_RD_REG32(baseAddr + TIMER_RELOAD);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERIntRawStatusGet(uint32_t baseAddr, uint32_t *pIntrRawStatus)
{
	int32_t retVal = -EINVAL;

	if ((baseAddr != (uint32_t)(0u)) && (pIntrRawStatus != NULL)) {
		/* Return the status of IRQSTATUS_RAW register */
		*pIntrRawStatus = HW_RD_REG32(baseAddr + TIMER_IRQSTATUS_RAW);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERIntStatusGet(uint32_t baseAddr, uint32_t *pIntrStatus)
{
	int32_t retVal = -EINVAL;

	if ((baseAddr != (uint32_t)(0u)) && (pIntrStatus != NULL)) {
		/* Return the status of IRQSTATUS register */
		*pIntrStatus = HW_RD_REG32(baseAddr + TIMER_IRQSTATUS);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERIntStatusClear(uint32_t baseAddr, uint32_t status)
{
	int32_t retVal = -EINVAL;

	if ((baseAddr != (uint32_t)(0u)) && (status != (uint32_t)(0u))) {
		/* IRQSTATUS is write-one-to-clear on AM654 DMTimer V2. */
		HW_WR_REG32(baseAddr + TIMER_IRQSTATUS, status);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERIntEnable(uint32_t baseAddr)
{
	int32_t retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		HW_WR_REG32(baseAddr + TIMER_IRQENABLE_SET, TIMER_IRQ_OVERFLOW);
		retVal = 0;
	}

	return retVal;
}

int32_t TIMERIntDisable(uint32_t baseAddr)
{
	int32_t retVal = -EINVAL;

	if (baseAddr != (uint32_t)(0u)) {
		HW_WR_REG32(baseAddr + TIMER_IRQENABLE_CLR, TIMER_IRQ_OVERFLOW);
		retVal = 0;
	}

	return retVal;
}
