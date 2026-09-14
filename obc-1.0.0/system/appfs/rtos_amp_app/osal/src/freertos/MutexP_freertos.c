/*
 * Copyright (c) 2021, Texas Instruments Incorporated
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

/* Stolen from ti/osal/src/freertos/MutexP_freertos.c */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "osal/osal.h"
#include "osal/soc/osal_soc.h"

/* Defined in Utils_freertos.c */
extern uint32_t  gOsalMutexAllocCnt, gOsalMutexPeak;

/*!
 *  @brief    Mutex structure
 */
typedef struct MutexP_freertos_s {
	bool used;
	StaticSemaphore_t semObj;
	SemaphoreHandle_t semHndl;
	uint32_t isRecursiveMutex;
} MutexP_freertos;

/* global pool of statically allocated mutex pools */
static MutexP_freertos gOsalMutexPFreeRtosPool[OSAL_FREERTOS_CONFIGNUM_MUTEX];

MutexP_Handle MutexP_create(MutexP_Object *mutexObj)
{
	MutexP_Handle ret_handle = NULL;
	MutexP_freertos *handle = NULL;
	MutexP_freertos *mutexPool;
	uint32_t i;
	uintptr_t key;
	uint32_t maxMutex;

	if (mutexObj == NULL) {
		ret_handle = NULL;
	} else {
		/* Pick up the internal static memory block */
		mutexPool = (MutexP_freertos *)&gOsalMutexPFreeRtosPool[0];
		maxMutex  = OSAL_FREERTOS_CONFIGNUM_MUTEX;

		if (gOsalMutexAllocCnt == 0U) {
			(void)memset((void *)gOsalMutexPFreeRtosPool,0,sizeof(gOsalMutexPFreeRtosPool));
		}

		key = HwiP_disable();

		for (i = 0; i < maxMutex; i++) {
			if (mutexPool[i].used == (bool)false) {
				mutexPool[i].used = (bool)true;
				/* Update statistics */
				gOsalMutexAllocCnt++;
				if (gOsalMutexAllocCnt > gOsalMutexPeak) {
					gOsalMutexPeak = gOsalMutexAllocCnt;
				}
				break;
			}
		}
		HwiP_restore(key);

		if (i < maxMutex) {
			/* Grab the memory */
			handle = (MutexP_freertos *)&mutexPool[i];
		}

		if (handle == NULL) {
			ret_handle = NULL;
		} else {
			handle->isRecursiveMutex = 1;
			handle->semHndl = xSemaphoreCreateRecursiveMutexStatic(&handle->semObj);

			if (handle->semHndl == NULL) {
				/* If there was an error reset the mutex object and return NULL. */
				key = HwiP_disable();
				handle->used = (bool)false;
				/* Found the osal task object to delete */
				if (gOsalMutexAllocCnt > 0U) {
					gOsalMutexAllocCnt--;
				}
				HwiP_restore(key);
				ret_handle = NULL;
			} else {
				mutexObj->object = (void *)handle;
				mutexObj->key = 0;
				ret_handle = (MutexP_Handle)mutexObj;
			}
		}
	}
	return ret_handle;
}

MutexP_Status MutexP_delete(MutexP_Handle handle)
{
	uintptr_t key;
	MutexP_Status ret = MutexP_OK;
	MutexP_Object *mutexObj = (MutexP_Object *)handle;
	MutexP_freertos *mutex;

	DebugP_assert(mutexObj != NULL);

	mutex = (MutexP_freertos *)mutexObj->object;

	if ((mutex != NULL) && (mutex->used == (bool)true)) {
		vSemaphoreDelete(mutex->semHndl);

		key = HwiP_disable();
		mutex->used = (bool)false;
		if (gOsalMutexAllocCnt > 0U) {
			gOsalMutexAllocCnt--;
		}
		HwiP_restore(key);
		ret = MutexP_OK;
	} else {
		ret = MutexP_FAILURE;
	}
	return ret;
}

MutexP_Status MutexP_lock(MutexP_Handle handle, uint32_t timeout)
{
	MutexP_Status ret = MutexP_OK;
	MutexP_Object *mutexObj = (MutexP_Object *)handle;
	MutexP_freertos *mutex;
	uint32_t isTaken = 0;

	DebugP_assert(mutexObj != NULL);

	mutex = (MutexP_freertos *)mutexObj->object;

	if ((mutex != NULL) && (mutex->used == (bool)true) && (mutex->isRecursiveMutex == 1U)) {
		if (xPortInIsrContext() == 0) {
			if (timeout == MutexP_WAIT_FOREVER) {
				isTaken = (uint32_t)xSemaphoreTakeRecursive(mutex->semHndl, portMAX_DELAY);
			} else {
				isTaken = (uint32_t)xSemaphoreTakeRecursive(mutex->semHndl, timeout);
			}
			if (isTaken != 0U) {
				ret = MutexP_OK;
			} else {
				ret = MutexP_TIMEOUT;
			}
		} else {
			ret = MutexP_FAILURE;
		}
	} else {
		ret = MutexP_FAILURE;
	}
	return ret;
}

MutexP_Status MutexP_unlock(MutexP_Handle handle)
{
	MutexP_Status ret = MutexP_OK;
	MutexP_Object *mutexObj = (MutexP_Object *)handle;
	MutexP_freertos *mutex;
	/* Note: timeout is not use */

	DebugP_assert(mutexObj != NULL);

	mutex = (MutexP_freertos *)mutexObj->object;

	if ((mutex != NULL) && (mutex->used == (bool)true) && (mutex->isRecursiveMutex == 1U)) {
		if (xPortInIsrContext() == 0) {
			(void)xSemaphoreGiveRecursive(mutex->semHndl);
			ret = MutexP_OK;
		} else {
			ret = MutexP_FAILURE;
		}
	} else {
		ret = MutexP_FAILURE;
	}
	return ret;
}
