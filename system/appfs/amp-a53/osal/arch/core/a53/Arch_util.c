/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/arch/core/a53/Arch_util.c */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "osal/src/nonos/Nonos_config.h"
#include "csl/csl_gic.h"	/* for GIC registers */

/* Local define */
#define HWIP_DEFAULT_PRIORITY    ((uint32_t)0xE0U)

/* Local hwi structures */

typedef struct HwiP_nonOs_s {
	bool used;
	Hwi_Struct hwi;
} HwiP_nonOs;

/* Local hwi structures */
static HwiP_nonOs hwiStructs[OSAL_NONOS_CONFIGNUM_HWI];
//static bool gTimestampFirstTime = (bool)true;
static bool gHwiInitialized = false;

#define GIC_BASE_ADDR		(CSL_GIC_BASE_ADDR)

void OsalArch_gicInit(void)
{
	uint64_t initStatus, mpidr;
	uint8_t  coreId;

	initStatus = Intc_isInitialized();
	if (initStatus == 0U) {
		mpidr = CSL_a53v8GetCpuId();
		/* AM62x MPIDR_EL1.Aff0 identifies the A53 core. */
		coreId = (uint8_t)(mpidr & (uint64_t)0xFFU);
		/*
		 * If not initialized via startup, initialze now
		 * initStatus FALSE, indicates that the CSL startup
		 * is not followed and possibly, some other schemes
		 * such as SBL/diag has started up and has done other
		 * required initializations, only pending initialization
		 * is Arm GIC
		 */

        /* Initialize the GIC V3 */	
		Intc_Init(coreId);
	}
}

/* This function enables the interrupt for a given interrupt number */
void OsalArch_enableInterrupt(uint32_t intrNum)
{
	Intc_SystemEnable((uint16_t)intrNum);
}

/* This function disables the interrupt for a given interrupt number */
void OsalArch_disableInterrupt(uint32_t intrNum)
{
	Intc_SystemDisable((uint16_t)intrNum);
}

/* Below function clears interrupt in the chip level */
void OsalArch_clearInterrupt(uint32_t intrNum)
{
}

/* Below function posts the interrupt */
int32_t OsalArch_postInterrupt(uint32_t intrNum)
{
	int32_t ret;
	ret = CSL_gicSetPendingIntr((uintptr_t)GIC_BASE_ADDR, (uint16_t)intrNum);
	CSL_a53v8DsbSy();
    return ret;
}

/* Below function globally disable interrupt in the chip level */
uintptr_t OsalArch_globalDisableInterrupt(void)
{
	return (uintptr_t)CSL_a53v8ArmGicv3HwiDisable();
}

/* Below function globally restore interrupt in the chip level */
void OsalArch_globalRestoreInterrupt (uintptr_t restoreValue)
{
	CSL_a53v8ArmGicv3HwiRestore((uint64_t)restoreValue);
}

/* Below function registers the interrupt for a given ISR */
HwiP_Handle OsalArch_HwiPCreate(uint32_t intrNum, HwiP_Fxn hwiFxn, const HwiP_Params *params)
{
	Hwi_Struct *hwi_handle = NULL;
	uint32_t i;
	uintptr_t key;
	uint16_t intrPrio;
//	uintptr_t temp;
	HwiP_nonOs *hwiPool;
	uint32_t maxHwi;
	HwiP_Handle handle = NULL;

#if 0
    /*
     * Check if user has specified any memory block to be used, which gets
     * the precedence over the internal static memory block
     */
	if (gOsal_HwAttrs.extHwiPBlock.base != 0U) {
		/* pick up the external memory block configured */
		hwiPool = (HwiP_nonOs *)gOsal_HwAttrs.extHwiPBlock.base;
		temp = ((uintptr_t)hwiPool) + gOsal_HwAttrs.extHwiPBlock.size;
		maxHwi = (uint32_t)(temp / (sizeof(Hwi_Struct)));
    } else
#endif
	{
		/* Pick up the internal static memory block */
		hwiPool = (HwiP_nonOs *)&hwiStructs[0];
		maxHwi = sizeof(hwiStructs) / sizeof(hwiStructs[0]);

		if (gHwiInitialized == false) {
			/* Initializing the first time */
			memset((void *)hwiStructs, 0, sizeof(hwiStructs));
			gHwiInitialized = true; 
		}
	}

	if (params != NULL) {
		key = OsalArch_globalDisableInterrupt();
		for (i = 0u; i < maxHwi; i++) {
			if (hwiPool[i].used == false) {
				hwiPool[i].used = true;
				break;
			}
		}
		OsalArch_globalRestoreInterrupt(key);

		if (i != maxHwi) {
			hwi_handle = &(hwiPool[i].hwi);
			handle = (HwiP_Handle)&hwiPool[i];
		} else {
			handle = NULL;
		}
	}

	if (hwi_handle != NULL) {
		/* Registering the Interrupt Service Routine(ISR). */
		Intc_IntRegister((uint16_t)intrNum, (IntrFuncPtr)hwiFxn, (void *)params->arg);

		/* Set the priority to default priority if priority is set un-initialized */

		if (params->intrPrio == HWIP_USE_DEFAULT_PRIORITY) {
			intrPrio = HWIP_DEFAULT_PRIORITY;
		} else {
			intrPrio = (uint16_t)params->intrPrio;
		}

		/* Setting the priority for the interrupts in INTC. */
		Intc_IntPrioritySet((uint16_t)intrNum, intrPrio);

		/* Set the trigger type */
		if ((params->intrType == OSAL_GIC_INTR_TYPE_HIGH_LEVEL) ||
			(params->intrType == OSAL_GIC_INTR_TYPE_LEVEL) ||
			(params->intrType == OSAL_GIC_INTR_TYPE_LOW_LEVEL)) {
			Intc_IntAssignLevelIntType((uint16_t)intrNum);
		}

		if ((params->intrType == OSAL_GIC_INTR_TYPE_RISING_EDGE) ||
            (params->intrType == OSAL_GIC_INTR_TYPE_EDGE) ||
            (params->intrType == OSAL_GIC_INTR_TYPE_FALLING_EDGE)) {
			Intc_IntAssignEdgeIntType((uint16_t)intrNum);
		}

#if 1
		/* By default, all interrupts are routed to CPU0. */
		Intc_IntSetAffinity((uint16_t)intrNum);	
#endif

		/* Return Non Zero value for the handle for A15 target */
		hwi_handle->intrNum = intrNum;

		/* Enabling the interrupt if configured */
		if (params->enableIntr == 1U) {
			OsalArch_enableInterrupt(intrNum);
		} else {
			OsalArch_disableInterrupt(intrNum);
		}
	}

	return handle;
}

/* Below function deletes the hwi created */
HwiP_Status OsalArch_HwiPDelete(HwiP_Handle handle)
{
	HwiP_nonOs *hwi_handle = (HwiP_nonOs *)handle;
	uintptr_t key;
	HwiP_Status retVal = HwiP_OK;

	/* Mark that handle as free */
	key = OsalArch_globalDisableInterrupt();
	if (hwi_handle->used) {
		hwi_handle->used = false;
		Intc_IntUnregister((uint16_t)(hwi_handle->hwi.intrNum));
	} else {
		retVal = HwiP_FAILURE;
	}
	OsalArch_globalRestoreInterrupt(key);

	return retVal;
}
