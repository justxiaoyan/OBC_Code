/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _OSAL_SOC_TOP_H_
#define _OSAL_SOC_TOP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* AM62x is the active target for this application. */
#include "osal/soc/am62x/osal_soc.h"

/*********************************************************************
 * @def OSAL_FREERTOS_CONFIGNUM_SEMAPHORE
 * If the need is more than the defaults, application would need to
 * suppliment the additional memory for OSAL using @ref Osal_setHwAttrs
 * API calls by setting the extended memory block for semaphores
 *********************************************************************/

/* Set the number of SemaphoreP_Handles for FREERTOS */
#ifndef OSAL_FREERTOS_CONFIGNUM_SEMAPHORE
#define OSAL_FREERTOS_CONFIGNUM_SEMAPHORE (OSAL_FREERTOS_MAX_SEMAPHOREP_PER_SOC)
#endif /* OSAL_FREERTOS_CONFIGNUM_SEMAPHORE */

/*********************************************************************
 * @def OSAL_FREERTOS_CONFIGNUM_TASK
 * If the need is more than the defaults, application would need to
 * suppliment the additional memory for OSAL using @ref Osal_setHwAttrs
 * API calls by setting the extended memory block for tasks
 *********************************************************************/

/* Set the number of SemaphoreP_Handles for FREERTOS */
#ifndef OSAL_FREERTOS_CONFIGNUM_TASK
#define OSAL_FREERTOS_CONFIGNUM_TASK (OSAL_FREERTOS_MAX_TASKP_PER_SOC)
#endif /* OSAL_FREERTOS_CONFIGNUM_TASK */

/*********************************************************************
 * @def OSAL_FREERTOS_CONFIGNUM_MUTEX
 * If the need is more than the defaults, application would need to
 * suppliment the additional memory for OSAL using @ref Osal_setHwAttrs
 * API calls by setting the extended memory block for mutex
 *********************************************************************/

/* Set the number of MutexP_Handles for FREERTOS */
#ifndef OSAL_FREERTOS_CONFIGNUM_MUTEX
#define OSAL_FREERTOS_CONFIGNUM_MUTEX (OSAL_FREERTOS_MAX_MUTEXP_PER_SOC)
#endif /* OSAL_FREERTOS_CONFIGNUM_MUTEX */

/*********************************************************************
 * @def OSAL_FREERTOS_CONFIGNUM_HWI
 * If the need is more than the defaults, application would need to
 * suppliment the additional memory for OSAL using @ref Osal_setHwAttrs
 * API calls by setting the extended memory block for HwiP
 *********************************************************************
 */
/* Set the number of HwiP_Handles */
#ifndef OSAL_FREERTOS_CONFIGNUM_HWI
#define OSAL_FREERTOS_CONFIGNUM_HWI (OSAL_FREERTOS_MAX_HWIP_PER_SOC)
#endif

/*********************************************************************
 * @def OSAL_FREERTOS_CONFIGNUM_TIMER
 * If the need is more than the defaults, application would need to
 * suppliment the additional memory for OSAL using @ref Osal_setHwAttrs
 * API calls by setting the extended memory block for timer
 *********************************************************************
 */
/* Set the number of TimerP_Handles for all boards */
#ifndef OSAL_FREERTOS_CONFIGNUM_TIMER
#define OSAL_FREERTOS_CONFIGNUM_TIMER (OSAL_FREERTOS_MAX_TIMERP_PER_SOC)
#endif

/*********************************************************************
 * @def OSAL_NONOS_CONFIGNUM_HWI
 * If the need is more than the defaults, application would need to
 * suppliment the additional memory for OSAL using @ref Osal_setHwAttrs
 * API calls by setting the extended memory block for HwiP
 *********************************************************************
 */
/* Set the number of HwiP_Handles */
#ifndef OSAL_NONOS_CONFIGNUM_HWI
#define OSAL_NONOS_CONFIGNUM_HWI (OSAL_NONOS_MAX_HWIP_PER_SOC)
#endif

/*********************************************************************
 * @def OSAL_NONOS_CONFIGNUM_TIMER
 * If the need is more than the defaults, application would need to
 * suppliment the additional memory for OSAL using @ref Osal_setHwAttrs
 * API calls by setting the extended memory block for timer
 *********************************************************************
 */
/* Set the number of TimerP_Handles for all boards */
#ifndef OSAL_NONOS_CONFIGNUM_TIMER
#define OSAL_NONOS_CONFIGNUM_TIMER (OSAL_NONOS_MAX_TIMERP_PER_SOC)
#endif

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_SOC_TOP_H_ */
