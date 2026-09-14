/*
 * Copyright (c) 2015-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== SemaphoreP_freertos.c ========
 */

/* Stolen from ti/osal/src/freertos/SemaphonreP_freertos.c */


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "osal/osal.h"
#include "osal/soc/osal_soc.h"
//#include "osal/src/nonos/Nonos_config.h" /* for NULL_PTR */

#ifndef NULL_PTR
#define NULL_PTR ((void *)0x0)
#endif

/* Defined in Utils_freertos.c */
extern uint32_t gOsalSemAllocCnt, gOsalSemPeak;

/*!
 *  @brief    Semaphore structure
 */
typedef struct SemaphoreP_freertos_s {
	bool               used;
	StaticSemaphore_t  semObj;
	SemaphoreHandle_t  semHndl;
} SemaphoreP_freertos;

/* global pool of statically allocated semaphore pools */
static SemaphoreP_freertos gOsalSemPfreertosPool[OSAL_FREERTOS_CONFIGNUM_SEMAPHORE];

int32_t SemaphoreP_constructBinary(SemaphoreP_freertos *handle, uint32_t initCount);
int32_t SemaphoreP_constructCounting(SemaphoreP_freertos *handle, uint32_t initCount, uint32_t maxCount);

/*
 *  ======== SemaphoreP_create ========
 */
SemaphoreP_Handle SemaphoreP_create(uint32_t count,
									const SemaphoreP_Params *params)
{
	SemaphoreP_Handle ret_handle;
	SemaphoreP_freertos *handle = (SemaphoreP_freertos *) NULL_PTR;
	uint32_t          i;
	uintptr_t         key;
//	uintptr_t         temp;
	SemaphoreP_freertos *semPool;
	uint32_t          maxSemaphores;
	int32_t           retVal;

#if 0
	/* Check if user has specified any memory block to be used, which gets
	 * the precedence over the internal static memory block
	 */
	if (gOsal_HwAttrs.extSemaphorePBlock.base != (uintptr_t)0U) {
		/* Pick up the external memory block configured */
		semPool        = (SemaphoreP_freertos *) gOsal_HwAttrs.extSemaphorePBlock.base;
		temp           = (uintptr_t) gOsal_HwAttrs.extSemaphorePBlock.size;
		maxSemaphores  = (uint32_t)(temp/(sizeof(SemaphoreP_freertos)));
	} else
#endif
	{
		/* Pick up the internal static memory block */
		semPool        = (SemaphoreP_freertos *) &gOsalSemPfreertosPool[0];
		maxSemaphores  = OSAL_FREERTOS_CONFIGNUM_SEMAPHORE;

		if (gOsalSemAllocCnt == 0U) {
			(void)memset( (void *)gOsalSemPfreertosPool,0,sizeof(gOsalSemPfreertosPool));
		}
	}

	key = HwiP_disable();

	for (i = 0; i < maxSemaphores; i++) {
		if (semPool[i].used == (bool)false) {
			semPool[i].used = (bool)true;
			/* Update statistics */
			gOsalSemAllocCnt++;
			if (gOsalSemAllocCnt > gOsalSemPeak) {
				gOsalSemPeak = gOsalSemAllocCnt;
			}
			break;
		}
	}
	HwiP_restore(key);

	if (i < maxSemaphores) {
		/* Grab the memory */
		handle = (SemaphoreP_freertos *) &semPool[i];
	}

	if (handle == NULL_PTR) {
		ret_handle = NULL_PTR;
	} else {
		if (params == NULL_PTR) {
			SemaphoreP_Params semParam;
			SemaphoreP_Params_init(&semParam);
			retVal = SemaphoreP_constructCounting(handle, count, semParam.maxCount);
		} else {
			if (params->mode == SemaphoreP_Mode_BINARY) {
				retVal = SemaphoreP_constructBinary(handle, count);
			} else {
				retVal = SemaphoreP_constructCounting(handle, count, params->maxCount);
			}
		}

		/* If there was an error reset the sem object and return NULL. */
		if (retVal != SemaphoreP_OK) {
			key = HwiP_disable();
			handle->used = (bool)false;
			/* Found the osal semaphore object to delete */
			if (gOsalSemAllocCnt > 0U) {
				gOsalSemAllocCnt--;
			}
			HwiP_restore(key);
			ret_handle = NULL_PTR;
		} else {
			ret_handle = ((SemaphoreP_Handle)handle);
		}
	}
	return ret_handle;
}

int32_t SemaphoreP_constructBinary(SemaphoreP_freertos *handle, uint32_t initCount)
{
	int32_t status;

	handle->semHndl = xSemaphoreCreateBinaryStatic(&handle->semObj);
	if (handle->semHndl == NULL) {
		status = SemaphoreP_FAILURE;
	} else {
		vQueueAddToRegistry(handle->semHndl, "Binary Sem (OSAL)");
		if (initCount == 1U) {
			/* Post a semaphore to increment initial count to 1 */
			(void)xSemaphoreGive(handle->semHndl);
		}
		status = SemaphoreP_OK;
	}

	return status;
}

int32_t SemaphoreP_constructCounting(SemaphoreP_freertos *handle, uint32_t initCount, uint32_t maxCount)
{
	int32_t status;

	handle->semHndl = xSemaphoreCreateCountingStatic(
								maxCount,
								initCount,
								&handle->semObj);
	if (handle->semHndl == NULL) {
		status = SemaphoreP_FAILURE;
	} else {
		vQueueAddToRegistry(handle->semHndl, "Counting Sem (OSAL)");
		status = SemaphoreP_OK;
	}

	return status;
}

/*
 *  ======== SemaphoreP_delete ========
 */
SemaphoreP_Status SemaphoreP_delete(SemaphoreP_Handle handle)
{
	uintptr_t   key;
	SemaphoreP_Status ret = SemaphoreP_OK;
	SemaphoreP_freertos *semaphore = (SemaphoreP_freertos *)handle;

	DebugP_assert((handle != NULL_PTR));

	if ((semaphore != NULL_PTR) && (semaphore->used==(bool)true)) {
		vSemaphoreDelete(semaphore->semHndl);

		key = HwiP_disable();
		semaphore->used = (bool)false;
		/* Found the osal semaphore object to delete */
		if (gOsalSemAllocCnt > 0U) {
			gOsalSemAllocCnt--;
		}
		HwiP_restore(key);
		ret = SemaphoreP_OK;
	} else {
		ret = SemaphoreP_FAILURE;
	}
	return (ret);
}

/*
 *  ======== SemaphoreP_Params_init ========
 */
void SemaphoreP_Params_init(SemaphoreP_Params *params)
{
	DebugP_assert((params != NULL_PTR));

	if (params != NULL_PTR) {
		params->mode = SemaphoreP_Mode_COUNTING;
		params->name = (char *) NULL_PTR;
		params->maxCount = 0xFFU;
    }
}

/*
 *  ======== SemaphoreP_pend ========
 */
SemaphoreP_Status SemaphoreP_pend(SemaphoreP_Handle handle, uint32_t timeout)
{
	uint32_t            isSemTaken;
	SemaphoreP_Status   ret_val;
	SemaphoreP_freertos *pSemaphore = (SemaphoreP_freertos *)handle;

	DebugP_assert((handle != NULL_PTR));

	if (xPortInIsrContext() == 1) {
		BaseType_t xHigherPriorityTaskWoken = 0;

		/* Timeout is ignored when in ISR mode */
		isSemTaken = (uint32_t)xSemaphoreTakeFromISR(pSemaphore->semHndl, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR((uint32_t)xHigherPriorityTaskWoken);
	} else {
		if (timeout == SemaphoreP_WAIT_FOREVER) {
			isSemTaken = (uint32_t)xSemaphoreTake(pSemaphore->semHndl, portMAX_DELAY);
		} else {
			isSemTaken = (uint32_t)xSemaphoreTake(pSemaphore->semHndl, timeout);
		}
	}

	if (isSemTaken != 0U) {
		ret_val = SemaphoreP_OK;
	} else {
		ret_val = SemaphoreP_TIMEOUT;
	}

	return ret_val;
}

/*
 *  ======== SemaphoreP_post ========
 */
SemaphoreP_Status SemaphoreP_post(SemaphoreP_Handle handle)
{
	SemaphoreP_freertos *pSemaphore = (SemaphoreP_freertos *)handle;

	DebugP_assert((handle != NULL_PTR));

	if(xPortInIsrContext() == 1) {
		BaseType_t xHigherPriorityTaskWoken = 0;

		(void)xSemaphoreGiveFromISR(pSemaphore->semHndl, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR((uint32_t)xHigherPriorityTaskWoken);
	} else {
		(void)xSemaphoreGive(pSemaphore->semHndl);
	}

	return (SemaphoreP_OK);
}

/*
 *  ======== SemaphoreP_postFromClock ========
 */
SemaphoreP_Status SemaphoreP_postFromClock(SemaphoreP_Handle handle)
{
	return (SemaphoreP_post(handle));
}

/*
 *  ======== SemaphoreP_postFromISR ========
 */
SemaphoreP_Status SemaphoreP_postFromISR(SemaphoreP_Handle handle)
{
    return (SemaphoreP_post(handle));
}

/*
 *  ======== SemaphoreP_getCount ========
 */
int32_t SemaphoreP_getCount(SemaphoreP_Handle handle)
{
	DebugP_assert((handle != NULL_PTR));
		
	SemaphoreP_freertos *pSemaphore = (SemaphoreP_freertos *)handle;

	return ((int32_t)uxSemaphoreGetCount(pSemaphore->semHndl));
}

SemaphoreP_Status SemaphoreP_reset(SemaphoreP_Handle handle)
{
	uint32_t isSemTaken = 0;
	SemaphoreP_Status ret_val = SemaphoreP_OK;
	SemaphoreP_freertos *pSemaphore = (SemaphoreP_freertos *)handle;

	DebugP_assert((handle != NULL_PTR));

	vTaskSuspendAll();
	do {
		isSemTaken = (uint32_t)xSemaphoreTake(pSemaphore->semHndl, 0);
	} while(isSemTaken != 0U);
	(void)xTaskResumeAll();

	return (ret_val);
}
