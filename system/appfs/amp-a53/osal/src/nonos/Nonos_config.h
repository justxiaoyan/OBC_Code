/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _OSAL_NONOS_CONFIG_H_
#define _OSAL_NONOS_CONFIG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/osal.h"
#include "osal/soc/osal_soc.h"

#include "csl/arch/a53/csl_a53.h"
#include "csl/arch/a53/interrupt.h"
#include "csl/arch/a53/csl_a53v8misc.h"

typedef struct hwi_struct {
	uint32_t intrNum;
} Hwi_Struct;

#ifndef NULL_PTR
#define NULL_PTR ((void *)0x0)
#endif

/*
 *  @brief    TimerP Information structure
 *
 *  Structure that contains the timer info table for the
 *  non-os use case for default initializations
 */
typedef struct TimerP_dmTimerDefault_s {
	const char  *name;       /*< Name of the timer instance. Memory must
                             persist for the life of the clock instance.
                             This can be used for debugging purposes, or
                             set to NULL if not needed. */
	uint32_t baseAddr;   /*< timer base address */
	uint32_t intrNum;     /*< timer Interrupt number */
} TimerP_dmTimerDefault;

/* Reference the default initializations for the dmtimers */
extern TimerP_dmTimerDefault gDmTimerPInfoTbl[TimerP_numTimerDevices];

/* This function returns the internal frequency set for the timer */
uint32_t TimerP_getDefaultFreq(uint32_t timerId);

/* This function enables the interrupt for a given interrupt number */
void OsalArch_enableInterrupt(uint32_t intrNum);

/* This function disables the interrupt for a given interrupt number */
void OsalArch_disableInterrupt(uint32_t intrNum);

/* Below function clears interrupt in the chip level */
void OsalArch_clearInterrupt(uint32_t intrNum);

/* Below function globally disable interrupt in the chip level */
uintptr_t OsalArch_globalDisableInterrupt(void);

/* Below function posts an interrupt */
int32_t OsalArch_postInterrupt(uint32_t intrNum);

/* Below function globally restore interrupt in the chip level */
void OsalArch_globalRestoreInterrupt (uintptr_t restoreValue);

/* Below function registers the interrupt for a given ISR */
HwiP_Handle OsalArch_HwiPCreate(uint32_t intrNum, HwiP_Fxn hwiFxn, const HwiP_Params *params);

/* Below function deletes/frees up the HwiP handle created */
HwiP_Status OsalArch_HwiPDelete(HwiP_Handle handle);

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_NONOS_CONFIG_H_  */
