/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/src/freertos/Utils_freertos.c */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Freertos includes */
#include "FreeRTOS.h"
#include "task.h"

#include "osal/src/nonos/Nonos_config.h"

/* Global Osal static memory status variables */
uint32_t gOsalTimerAllocCnt = 0U, gOsalTimerPeak = 0U;
uint32_t gOsalHwiAllocCnt = 0U, gOsalHwiPeak = 0U;
uint32_t gOsalMutexAllocCnt = 0U, gOsalMutexPeak = 0U;
uint32_t gOsalSemAllocCnt = 0U, gOsalSemPeak = 0U;

volatile bool Osal_DebugP_Assert_Val = true;

/*
 *  ======== Osal_DebugP_assert ========
 */
void Osal_DebugP_assert(int32_t expression, const char *file, int32_t line)
{
	(void)file;
	(void)line;
    
	if (expression != 0) {
		while (Osal_DebugP_Assert_Val == (bool)true) {}
	}
}
