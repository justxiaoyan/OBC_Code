/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/src/freertos/HwiP_freertos.c */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* Freertos includes */
#include "FreeRTOS.h"
#include "task.h"	/* for xTaskGetSchedulerState(), taskSCHEDULER_NOT_STARTED */

#include "osal/src/nonos/Nonos_config.h"

/* Defined in Utils_freertos.c */
extern uint32_t  gOsalHwiAllocCnt, gOsalHwiPeak;

/*
 *  ======== HwiP_Params_init ========
 */
void HwiP_Params_init(HwiP_Params *params)
{
	params->name     = (char *)NULL_PTR;
	params->arg      = 0;
	params->intrPrio = HWIP_USE_DEFAULT_PRIORITY;
//	params->eventId  = 0;
	params->enableIntr = 1;
	params->intrType = (uint32_t)OSAL_GIC_INTR_TYPE_LEVEL;
//#if !((__ARM_ARCH == 7) && (__ARM_ARCH_PROFILE == 'R'))
//	{
//		Osal_HwAttrs hwAttrs;
//		(void)Osal_getHwAttrs(&hwAttrs);
//		if (hwAttrs.hwAccessType == OSAL_HWACCESS_UNRESTRICTED)
//		{
//			/* Do GIC init only in the case of unrestricted hw access */
//			OsalArch_gicInit();
//		}
//	}
//#endif
}

/*
 *  ======== HwiP_create ========
 */
HwiP_Handle HwiP_create(uint32_t intrNum, HwiP_Fxn hwiFxn, const HwiP_Params *params)
{
	HwiP_Handle handle;
	handle = OsalArch_HwiPCreate(intrNum, hwiFxn, params);

	/* Update statistics for successful allocation */
	if (handle != NULL_PTR) {
		gOsalHwiAllocCnt++;
		if (gOsalHwiAllocCnt > gOsalHwiPeak) {
			gOsalHwiPeak = gOsalHwiAllocCnt;
		}
	}
	return handle;
}

#if 0
/*
 *  ======== HwiP_createDirect ========
 */
HwiP_Handle HwiP_createDirect(uint32_t intrNum, HwiP_DirectFxn hwiFxn,
                              const HwiP_Params *params)
{
	HwiP_Handle handle;
#if defined (BUILD_MCU)
	handle = OsalArch_HwiPCreateDirect(intrNum, hwiFxn, params);
#else
	handle = NULL_PTR;
#endif

	/* Update statistics for successful allocation */
	if (handle != NULL_PTR) {
		gOsalHwiAllocCnt++;
		if (gOsalHwiAllocCnt > gOsalHwiPeak) {
			gOsalHwiPeak = gOsalHwiAllocCnt;
		}
	}
	return handle;
}
#endif

/*
 *  ======== HwiP_delete ========
 */
HwiP_Status HwiP_delete(HwiP_Handle handle)
{
	HwiP_Status status;

	OSAL_Assert((handle == NULL_PTR));

	if (handle != NULL_PTR) {
		status = OsalArch_HwiPDelete(handle);

		if (status == HwiP_OK) {
			if (gOsalHwiAllocCnt > 0U) {
				gOsalHwiAllocCnt--;
			}
		}
	} else {
		status = HwiP_FAILURE;
	}

	return status;
}

int32_t HwiP_post(uint32_t intrNum)
{
	return OsalArch_postInterrupt(intrNum);
}

/*
 *  ======== HwiP_clearInterrupt ========
 */
void HwiP_clearInterrupt(uint32_t intrNum)
{
	OsalArch_clearInterrupt(intrNum);
}

/*
 *  ======== HwiP_disableInterrupt ========
 */
void HwiP_disableInterrupt(uint32_t intrNum)
{
	OsalArch_disableInterrupt(intrNum);
}

/*
 *  ======== HwiP_enableInterrupt ========
 */
void HwiP_enableInterrupt(uint32_t intrNum)
{
	OsalArch_enableInterrupt(intrNum);
}

/*
 *  ======== HwiP_disable ========
 */
uintptr_t HwiP_disable(void)
{
	uintptr_t key = (uintptr_t)NULL_PTR;;

	if (( xPortInIsrContext() == 1 ) ||
	    ( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED )) {
		key = OsalArch_globalDisableInterrupt();
	} else {
		portENTER_CRITICAL();
	}

	return key;
}

/*
 *  ======== HwiP_restore ========
 */
void HwiP_restore(uintptr_t key)
{
	if (( xPortInIsrContext() == 1 ) ||
	    ( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED )) {
		OsalArch_globalRestoreInterrupt(key);
	} else {
		portEXIT_CRITICAL();
	}

	return;
}
