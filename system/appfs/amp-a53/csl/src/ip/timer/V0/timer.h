/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_TIMER_H_
#define _CSL_TIMER_H_

#include <stdint.h>

#include "csl/hw_types.h"
#include "csl/cslr_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
/**
 * Timer_Prescaler_Clk_Values
 * Values that can be passed to TIMERPreScalerClkEnable as ptv so as to derive
 *        Pre-Scalar clock from timer clock.
 *  @{
**/

/* Value used to divide timer clock by 16 */
#define TIMER_PRESCALER_CLK_DIV_BY_16		(1 << TIMER_CONTROL_TIMER_PRESCALER_SHIFT)

/* Value used to divide timer clock by 256 */
#define TIMER_PRESCALER_CLK_DIV_BY_256		(2 << TIMER_CONTROL_TIMER_PRESCALER_SHIFT)

/**
 * Timer_Mode_Values
 * Values that can be passed to TIMERModeConfigure as timerMode.
 *  @{
**/
/* Value used to configure the timer in one-shot mode */
#define TIMER_ONESHOT_MODE				(1U)
/* Value used to configure the timer in auto-reload (period) mode */
#define TIMER_RELOAD_MODE				(2U)
/* Value used to configure the timer in free-run mode */
#define TIMER_FREERUN_MODE				(0U)

/**
 * Timer_Size_Values
 * Values that can be passed to TIMERCounterSizeConfigure as counterSize.
 *  @{
**/
#define TIMER_COUNTER_SIZE_16_BIT		(0U)
#define TIMER_COUNTER_SIZE_32_BIT		(1 << TIMER_CONTROL_TIMER_SIZE_SHIFT)

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

int32_t TIMEREnable(uint32_t baseAddr);
int32_t TIMERDisable(uint32_t baseAddr);
int32_t TIMERModeConfigure(uint32_t baseAddr, uint32_t timerMode);
int32_t TIMERCounterSizeConfigure(uint32_t baseAddr, uint32_t counterSize);
int32_t TIMERPreScalerClkEnable(uint32_t baseAddr, uint32_t preClkTimeValue);
int32_t TIMERPreScalerClkDisable(uint32_t baseAddr);
int32_t TIMERCounterSet(uint32_t baseAddr, uint32_t counter);
int32_t TIMERCounterGet(uint32_t baseAddr, uint32_t *pCounterVal);
int32_t TIMERReloadSet(uint32_t baseAddr, uint32_t reload);
int32_t TIMERReloadGet(uint32_t baseAddr, uint32_t *pReloadVal);
int32_t TIMERIntRawStatusGet(uint32_t baseAddr, uint32_t *pIntrRawStatus);
int32_t TIMERIntStatusGet(uint32_t baseAddr, uint32_t *pIntrStatus);
int32_t TIMERIntStatusClear(uint32_t baseAddr, uint32_t status);
int32_t TIMERIntEnable(uint32_t baseAddr);
int32_t TIMERIntDisable(uint32_t baseAddr);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_TIMER_H_  */
