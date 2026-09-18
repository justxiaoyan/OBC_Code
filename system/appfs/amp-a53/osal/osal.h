/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _OSAL_H_
#define _OSAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/TaskP.h"
#include "osal/DebugP.h"
#include "osal/HwiP.h"
#include "osal/TimerP.h"
#include "osal/LoadP.h"
#include "osal/RegisterIntr.h"
#include "osal/MutexP.h"
#include "osal/SemaphoreP.h"

/* Internal function for assert */
extern void Osal_DebugP_assert(int32_t expression, const char *file, int32_t line);

/*!
 *  @brief  Assert checking function
 *
 *  If the expression is evaluated to true, the API does nothing.
 *  If it is evaluated to false, the underlying RTOS port implementation
 *  handles the assert via its mechanisms.
 *
 *  @param  expression Expression to evaluate
 */
/* in real code use TI's version of OSAL_Assert (which is also fatal for OSAL_Assert(1)*/
#define OSAL_Assert(expression) (Osal_DebugP_assert((int32_t)((expression)?1:0),\
                                                  __FILE__, __LINE__))
#if 0
/*!
 *  @brief  Osal hw access type 
 *
 */
typedef enum {
	OSAL_HWACCESS_UNRESTRICTED, /*!< No restriction on hardware access or configuration */
	OSAL_HWACCESS_RESTRICTED    /*!< Restrictive access to hardware such as GIC */
} Osal_HwAccessType;

/*!
 *  @brief  Osal memory address range
 *
 */
typedef struct Osal_memRange_s {
	uintptr_t    base;          /*!< Start address value */
	uint32_t     size;          /*!< size in bytes */
} Osal_memRange;

/*!
 *  @brief  Osal hw attributes structure
 *
 */
typedef struct Osal_HwAttrs_s {
	/*! CPU frequency in KHz */
	int32_t  cpuFreqKHz;
	/*! External Clock value in KHz */
	int32_t  extClkKHz;
#if defined(gnu_targets_arm_A15F)
	/*! Set Target processor list */
	uint32_t a15TargetProcMask;
#endif
#ifdef _TMS320C6X
	/*! Interrupt numbers for Event combiner groups (0-3) */
	int32_t ECM_intNum[4];
#endif
	/*! Hardware access type */
	Osal_HwAccessType hwAccessType;
	/*!
	 * Delay timer base address:
	 * Delay timer base configuration is applicable for only AM335x/AM437x SoCs
	 * It is not applicable and should not be set for other SoCs
	 * Osal setHwAttrs API would return failure (osal_UNSUPPORTED) if attempted to
	 * be set for SoCs other than AM335x and AM437x.
	 */
	uintptr_t     osalDelayTimerBaseAddr;
	/*! Semaphore external memory control block */
	Osal_memRange  extSemaphorePBlock;
	/*! Hwi external memory control block */
	Osal_memRange  extHwiPBlock;
} Osal_HwAttrs;
#endif

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_H_ */
