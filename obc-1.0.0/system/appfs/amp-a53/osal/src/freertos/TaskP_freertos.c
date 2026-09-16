/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/src/freertos/TaskP_freertos.c */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "osal/osal.h"
#include "osal/soc/osal_soc.h" /* for OSAL_FREERTOS_CONFIGNUM_TASK */

/* Defined in LoadP_freertos.c */
extern void LoadP_addTask(TaskP_Handle handle, uint32_t taskId);
extern void LoadP_removeTask(uint32_t taskId);

/**
 * \brief Value to be used for lowest priority task
 */
#define TaskP_PRIORITY_LOWEST       (0)

/**
 * \brief Value to be used for highest priority task
 */
#define TaskP_PRIORITY_HIGHEST      (configMAX_PRIORITIES - 1)

/*
 * Defines the prototype of the task main functions.
 */
typedef void (*TaskP_mainFunction_t)(void *arg0, void *arg1);

/*!
 *  @brief    Task structure
 */
typedef struct TaskP_freertos_s {
	bool used;
	uint32_t taskId;
	StaticTask_t *taskObj;
	TaskHandle_t task;
	TaskP_mainFunction_t taskFxn;
	void *arg0;
	void *arg1;
	bool terminated;
} TaskP_freertos;

/* task obj is too large to keep in structure. So keep it in global array. */
static StaticTask_t gOsalFreertosTaskObjPool[OSAL_FREERTOS_CONFIGNUM_TASK];
/* global pool of statically allocated task pools */
static TaskP_freertos gOsalTaskPfreertosPool[OSAL_FREERTOS_CONFIGNUM_TASK];

uint32_t gOsalTaskAllocCnt = 0U, gOsalTaskPeak = 0U;

static void TaskP_Function(void *arg)
{
	TaskP_freertos *handle = (TaskP_freertos *)(arg);

	/* Call the application function. */
	(*handle->taskFxn)(handle->arg0, handle->arg1);

	/* Task Fxn completed execution. */
	handle->terminated = true;
	/* Put vTaskSuspend in a loop just in case some calls vTaskResume, it will go back to suspend. */
	while (handle->terminated) {
		vTaskSuspend(NULL);
	}
}

/*
 *  ======== TaskP_create ========
 */
TaskP_Handle TaskP_create(TaskP_Fxn taskFxn, const TaskP_Params *params)
{
	TaskP_freertos *handle = NULL;
	TaskP_freertos *taskPool;
	uint32_t i, maxTasks;
	uintptr_t key;
	uint8_t priority;

	DebugP_assert(taskFxn != NULL);
	DebugP_assert(params != NULL);
	DebugP_assert(params->stack != NULL);

	/* Pick up the internal static memory block */
	taskPool = (TaskP_freertos *)&gOsalTaskPfreertosPool[0];
	maxTasks = OSAL_FREERTOS_CONFIGNUM_TASK;

	if (gOsalTaskAllocCnt == 0U) {
		memset((void *)gOsalTaskPfreertosPool, 0, sizeof(gOsalTaskPfreertosPool));
		memset((void *)gOsalFreertosTaskObjPool, 0, sizeof(gOsalFreertosTaskObjPool));
	}

	key = HwiP_disable();

	for (i = 0; i < maxTasks; i++) {
		if (taskPool[i].used == false) {
			taskPool[i].used = true;
			taskPool[i].taskObj = &gOsalFreertosTaskObjPool[i];
			/* Update statistics */
			gOsalTaskAllocCnt++;
			if (gOsalTaskAllocCnt > gOsalTaskPeak) {
				gOsalTaskPeak = gOsalTaskAllocCnt;
			}
			break;
		}
	}
	HwiP_restore(key);

    if (i < maxTasks) {
		/* Grab the memory */
		handle = &taskPool[i];
		handle->taskId = i;
	}

	if (handle != NULL) {
		priority = params->priority;
		/* If prority is out of range, adjust to bring it in range */
		if (priority > TaskP_PRIORITY_HIGHEST) {
			priority = TaskP_PRIORITY_HIGHEST;
		}
		if (priority <= TaskP_PRIORITY_LOWEST) {
			priority = TaskP_PRIORITY_LOWEST;
		}
		/* Store the task function and args in taskP object. */
		handle->taskFxn = (TaskP_mainFunction_t)(taskFxn);
		handle->arg0 = params->arg0;
		handle->arg1 = params->arg1;
		handle->terminated = false;

		handle->task = xTaskCreateStatic((TaskFunction_t)TaskP_Function, /* Pointer to the function that implements the task. */
									  (char *)params->name,              /* Text name for the task.  This is to facilitate debugging only. */
									  (UBaseType_t)params->stackSize / (sizeof(UBaseType_t)),  /* Stack depth in units of StackType_t typically uint32_t on 32b CPUs */
									  handle,       /* task specific args */
									  (UBaseType_t)priority,   /* task priority, 0 is lowest priority, configMAX_PRIORITIES-1 is highest */
									  (StackType_t*)params->stack,      /* pointer to stack base */
									  handle->taskObj); /* pointer to statically allocated task object memory */
		if (handle->task == NULL) {
			/* If there was an error reset the task object and return NULL. */
			key = HwiP_disable();
			handle->used = false;
			/* Found the osal task object to delete */
			if (gOsalTaskAllocCnt > 0U) {
				gOsalTaskAllocCnt--;
			}
			HwiP_restore(key);
			handle = NULL;
		} else {
			LoadP_addTask((TaskP_Handle)handle, handle->taskId);
#if (configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0)
			vTaskSetThreadLocalStoragePointer(handle->task, 0, params->userData);
#endif
		}
	}

	return (TaskP_Handle)handle;
}

/*
 *  ======== TaskP_delete ========
 */
TaskP_Status TaskP_delete(TaskP_Handle handle)
{
	TaskP_freertos *handle_ = (TaskP_freertos *)handle;
	TaskP_Status ret = TaskP_OK;
	uintptr_t key;

	if ((handle_ != NULL) && (handle_->used == true)) {
		TaskHandle_t xTask = xTaskGetCurrentTaskHandle();

		if (xTask == handle_->task) {
			/* This is task deleting itself. */
			DebugP_log0("Warning: vTaskDelete will not return when the task is deleting itself.\n"
						"Resource freeing should be handled in deleteHookcallback which is not done currently\n"
						"So there will be resource leak\n");
		}

		vTaskDelete(handle_->task);
		LoadP_removeTask(handle_->taskId);

		key = HwiP_disable();
		/*
		 * In FreeRTOS, task is deleted in the idle task.
         * So the TaskObj should not be memset to 0.
         */
		/* (void)memset((void *)handle_->taskObj, 0, sizeof(handle_->taskObj)); */
		handle_->used = false;
		handle_->taskObj = NULL;
		handle_->task = NULL;
		handle_->taskFxn = NULL;
		handle_->arg0 = NULL;
		handle_->arg1 = NULL;

		/* Found the osal task object to delete */
		if (gOsalTaskAllocCnt > 0U) {
			gOsalTaskAllocCnt--;
		}
		HwiP_restore(key);

		ret = TaskP_OK;
	} else {
		ret = TaskP_FAILURE;
	}

	return ret;
}

/*
 *  ======== TaskP_Params_init ========
 */
void TaskP_Params_init(TaskP_Params *params)
{
	if (params != NULL) {
		params->name = (const char *)"FREERTOS_TASK";
		params->stackSize = 0;
		params->stack = NULL;
		params->priority = (TaskP_PRIORITY_HIGHEST - TaskP_PRIORITY_LOWEST) / 2;
		params->arg0 = NULL;
		params->arg1 = NULL;
		params->userData = NULL;
		params->taskPrivilege = 0U; /* Not Used */
	}
}

void TaskP_sleep(uint32_t timeout)
{
	vTaskDelay(timeout);
}

void TaskP_sleepInMsecs(uint32_t timeoutInMsecs)
{
	uint32_t ticks;

	/* portTICK_PERIOD_MS is in units of msecs */
	ticks = timeoutInMsecs / portTICK_PERIOD_MS;

	vTaskDelay(ticks);
}

void TaskP_setPrio(TaskP_Handle handle, uint32_t priority)
{
	TaskP_freertos *handle_ = (TaskP_freertos *)handle;

	DebugP_assert(handle_ != NULL);
	vTaskPrioritySet(handle_->task, priority);
}

TaskP_Handle TaskP_self(void)
{
	TaskHandle_t xTask;
	TaskP_Handle handle = NULL;
	uint32_t i;

	xTask = xTaskGetCurrentTaskHandle();
	if (xTask != NULL) {
		/* Now get the corresponding TaskP Handle */
		for (i = 0; i < OSAL_FREERTOS_CONFIGNUM_TASK; i++) {
			if ((gOsalTaskPfreertosPool[i].used == true) &&
				(gOsalTaskPfreertosPool[i].task == xTask)) {
				handle = (TaskP_Handle)(&gOsalTaskPfreertosPool[i]);
				break;
			}
		}
	}

	return handle;
}

TaskP_Handle TaskP_selfmacro(void)
{
	/*
	 * For freertos task self is not implemented as inline macro.
	 * So call the TaskP_self API itself.
	 */
	return TaskP_self();
}

void TaskP_yield(void)
{
	taskYIELD();
}

uint32_t TaskP_isTerminated(TaskP_Handle handle)
{
	uint32_t isTaskTerminated = 0;
	TaskP_freertos *handle_ = (TaskP_freertos *)handle;

	DebugP_assert(handle_ != NULL);
	if ((handle_->terminated == true) ||
		(eTaskGetState(handle_->task) == eDeleted)) {
		isTaskTerminated = 1;
	} else {
		isTaskTerminated = 0;
	}

	return isTaskTerminated;
}

uint32_t TaskP_disable(void)
{
	vTaskSuspendAll();

	return 0;
}

void TaskP_restore(uint32_t key)
{
 	(void)xTaskResumeAll();
}

TaskHandle_t TaskP_getFreertosHandle(TaskP_Handle handle)
{
	TaskP_freertos *handle_ = (TaskP_freertos *)handle;

	DebugP_assert(handle_ != NULL);
	DebugP_assert(handle_->used != false);

	return handle_->task;
}

uint32_t TaskP_getTaskId(TaskP_Handle handle)
{
	TaskP_freertos *handle_ = (TaskP_freertos *)handle;

	DebugP_assert(handle_ != NULL);
	DebugP_assert(handle_->used != false);

	return handle_->taskId;
}

void OS_init( void )
{
	/* Nothing to do */
}

void OS_start(void)
{
	vTaskStartScheduler();
}

void OS_stop(void)
{
	vTaskEndScheduler();
}
