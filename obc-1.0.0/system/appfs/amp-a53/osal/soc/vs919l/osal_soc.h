/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _OSAL_SOC_VS919L_H_
#define _OSAL_SOC_VS919L_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This platform supports two clock frequencies: 3MHz and 32kHz.
 * The default frequency is 3MHz.
 */
#define TIMERP_TIMER_FREQ			((uint32_t) 3000000)

#define TimerP_numTimerDevices		((uint32_t) 16U )
#define TimerP_ANY_MASK 			((uint32_t) 0xFFFF)
#define TimerP_AVAILABLE_MASK		((uint32_t)(0xFFFF))

/* Max number of various modules for NonOs */
//#define OSAL_NONOS_MAX_SEMAPHOREP_PER_SOC   ((uint32_t) 80U)
#define OSAL_NONOS_MAX_HWIP_PER_SOC         ((uint32_t) 64U)
#define OSAL_NONOS_MAX_TIMERP_PER_SOC       (TimerP_numTimerDevices)

/* Max number of various modules for FreeRTOS */
#define OSAL_FREERTOS_MAX_SEMAPHOREP_PER_SOC ((uint32_t) 256U)
#define OSAL_FREERTOS_MAX_HWIP_PER_SOC       ((uint32_t) 64U)
#define OSAL_FREERTOS_MAX_TIMERP_PER_SOC     (TimerP_numTimerDevices)
#define OSAL_FREERTOS_MAX_TASKP_PER_SOC      ((uint32_t) 128U)
#define OSAL_FREERTOS_MAX_MUTEXP_PER_SOC     ((uint32_t) 32U)

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_SOC_VS919L_H_ */
