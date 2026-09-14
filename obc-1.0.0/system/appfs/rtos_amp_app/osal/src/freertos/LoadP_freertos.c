/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/src/freertos/LoadP_freertos.c */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "osal/LoadP.h"
#include "osal/soc/osal_soc.h" /* for OSAL_FREERTOS_CONFIGNUM_TASK */

typedef struct LoadP_taskLoadObj_s {
	bool used;
	TaskP_Handle handle;
	uint64_t threadTime;
	uint32_t lastUpdate_threadTime;
} LoadP_taskLoadObj;

typedef struct LoadP_freertos_s {   
	LoadP_taskLoadObj taskLoadObj[OSAL_FREERTOS_CONFIGNUM_TASK];
	uint64_t idlTskTime;
	uint64_t totalTime;
	uint32_t lastUpdate_idlTskTime;
	uint32_t lastUpdate_totalTime;
} LoadP_freertos;

/* Defined in TaskP_freertos.c */
extern TaskHandle_t TaskP_getFreertosHandle(TaskP_Handle handle);
extern uint32_t TaskP_getTaskId(TaskP_Handle handle);

static LoadP_freertos gLoadP_freertos;
static bool gLoadP_initDone = false;

static uint32_t LoadP_calcCounterDiff(uint32_t cur, uint32_t last)
{
	uint32_t delta;

	if (cur >= last) {
		delta = cur - last;
	} else {
		delta = (0xFFFFFFFFU - last) + cur;
	}
	return delta;
}

static uint32_t LoadP_calcPercentLoad(uint64_t threadTime, uint64_t totalTime)
{
	uint32_t percentLoad;

	percentLoad = (uint32_t)(threadTime / (totalTime / 100U));

	if (percentLoad > 100U) {
		percentLoad = 100U;
	}

	return percentLoad;
}

void LoadP_addTask(TaskP_Handle handle, uint32_t taskId)
{
	LoadP_taskLoadObj *taskObj = &gLoadP_freertos.taskLoadObj[taskId];
    
	if (!gLoadP_initDone) {
		memset((void *)&gLoadP_freertos, 0, sizeof(gLoadP_freertos));
		gLoadP_initDone = true;
	}

	taskObj->used = true;
	taskObj->handle = handle;
	taskObj->threadTime = 0U;
	taskObj->lastUpdate_threadTime = 0U;
}

void LoadP_removeTask(uint32_t taskId)
{
	LoadP_taskLoadObj *taskObj = &gLoadP_freertos.taskLoadObj[taskId];
	taskObj->used = false; 
}

void LoadP_reset(void)
{
	uint32_t i;
	LoadP_taskLoadObj *taskObj;

	vTaskSuspendAll();

	gLoadP_freertos.idlTskTime = 0U;
	gLoadP_freertos.totalTime = 0U;

	for (i = 0; i < OSAL_FREERTOS_CONFIGNUM_TASK; i++) {
		taskObj = &gLoadP_freertos.taskLoadObj[i];

		if (taskObj->used == true) {
			taskObj->threadTime = 0U;
		}
	}
	xTaskResumeAll();
}

LoadP_Status LoadP_getTaskLoad(TaskP_Handle handle, LoadP_Stats *stats)
{
	LoadP_taskLoadObj *taskObj;
	LoadP_Status retVal = LoadP_FAILURE;
	TaskStatus_t stats_;
	uint32_t taskId;

	if ((stats != NULL) && (handle != NULL)) {
		vTaskSuspendAll();

		LoadP_update();

		taskId = TaskP_getTaskId(handle);
		taskObj = &gLoadP_freertos.taskLoadObj[taskId];

		if ((taskObj->used == true) && (taskObj->handle == handle)) {
			vTaskGetInfo(TaskP_getFreertosHandle(taskObj->handle), &stats_, pdFALSE, eRunning);
			stats->threadTime = taskObj->threadTime;
			stats->totalTime = gLoadP_freertos.totalTime;
			stats->percentLoad = LoadP_calcPercentLoad(taskObj->threadTime, gLoadP_freertos.totalTime);
			stats->name = stats_.pcTaskName;
			retVal = LoadP_OK;
		}

		xTaskResumeAll();
	}

	return retVal;
}

uint32_t LoadP_getCPULoad(void)
{
	uint32_t cpuLoad;

	LoadP_update();

	vTaskSuspendAll();

	cpuLoad = 100U - LoadP_calcPercentLoad(gLoadP_freertos.idlTskTime, gLoadP_freertos.totalTime);

	xTaskResumeAll();

	return cpuLoad;
}

void LoadP_update(void)
{
	uint32_t i;
	uint32_t time;
	uint32_t delta;
	LoadP_taskLoadObj *taskObj;
	TaskStatus_t stats;

	vTaskSuspendAll();

	if (!gLoadP_initDone) {
		memset((void *)&gLoadP_freertos, 0, sizeof(gLoadP_freertos));
		gLoadP_initDone = false;
	}

	/* Idle Task Update */
	time = (uint32_t)ulTaskGetIdleRunTimeCounter();
	delta = LoadP_calcCounterDiff(time, gLoadP_freertos.lastUpdate_idlTskTime);
	gLoadP_freertos.lastUpdate_idlTskTime = time;
	gLoadP_freertos.idlTskTime += delta;

	/* Total Time Update */
	time = (uint32_t)uiPortGetRunTimeCounterValue();
	delta = LoadP_calcCounterDiff(time, gLoadP_freertos.lastUpdate_totalTime);
	gLoadP_freertos.lastUpdate_totalTime = time;
	gLoadP_freertos.totalTime += delta;

	/* All tasks Update */
	for (i = 0; i < OSAL_FREERTOS_CONFIGNUM_TASK; i++) {
		taskObj = &gLoadP_freertos.taskLoadObj[i];

		if ((taskObj->used == true) && (taskObj->handle != NULL)) {
			vTaskGetInfo(TaskP_getFreertosHandle(taskObj->handle),
						  &stats,
						  pdFALSE,
						  eRunning);

			delta = LoadP_calcCounterDiff(stats.ulRunTimeCounter, taskObj->lastUpdate_threadTime);
			taskObj->lastUpdate_threadTime = stats.ulRunTimeCounter;
			taskObj->threadTime += delta;
		}
	}

	xTaskResumeAll();
}

/* Caller: vApplicationIdleHook() */
void vApplicationLoadHook(void)
{
	static uint32_t t0 = 0U; /* Last check time */
	static uint32_t timeElapsed = 0U; /* Elapsed time until last update */
	uint32_t t1 = uiPortGetRunTimeCounterValue(); /* Current time */
	uint32_t delta = t1 - t0;

	t0 = t1; /* Update last check time */

	/* Accumulate elapsed time until last update */
	timeElapsed += delta;

	/* Check for window */
	if ((timeElapsed) > (uint32_t)configLOAD_WINDOW_IN_MS) {
		/* Update load and reset elapsed time */
		LoadP_update();
		timeElapsed = 0;
	}
}
