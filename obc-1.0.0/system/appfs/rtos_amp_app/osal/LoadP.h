/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/LoadP.h */

/** ============================================================================
 *  @file       LoadP.h
 *
 *  @brief      Load module to measure CPU Load and Task Load for the RTOS Porting Interface
 *
 *
 *  ============================================================================
 */

#ifndef _OSAL_LOADP_H_
#define _OSAL_LOADP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/TaskP.h"

/*!
 *  @brief    Status codes for LoadP APIs
 */
typedef enum LoadP_Status_e {
	/*! API completed successfully */
	LoadP_OK         = 0,
	/*! API failed */
	LoadP_FAILURE    = (-(int32_t)1)
} LoadP_Status;

/*!
 * @brief Load statistics info
 */
typedef struct LoadP_Stats_s {
	const char *name;          /**< Name of the task */
	uint64_t threadTime;    /*!< Runtime for the thread */
	uint64_t totalTime;     /*!< Total Runtime */
	uint32_t percentLoad;   /*!< Percentage load of the thread */
} LoadP_Stats;

/*!
 *  @brief  Function to Reset load statistics
 *
 *          Until load statistics is reset the load statistics keep getting accumulated.
 */
extern void LoadP_reset(void);

/*!
 *  @brief  Function to get task load statistics
 *        
 *  @param  taskHandle [in]  A TaskP_Handle returned from \ref TaskP_create
 *  @param  stats      [out] Returned Task Load stats  
 * 
 *  @return Status of the function
 *    - LoadP_OK: LoadP_Stats populated with the task load statistics
 *    - LoadP_FAILURE: Failed to populate LoadP_Stats with the task load statistics
 */
extern LoadP_Status LoadP_getTaskLoad(TaskP_Handle handle, LoadP_Stats *stats);

/*!
 *  @brief  Function to get CPU load in percentage
 *          
 *  @return CPU load in percentage
 */
extern uint32_t LoadP_getCPULoad(void);

/*!
 *  @brief  Update load statistics
 *          
 *          This updates task load statistics for all tasks created with TaskP_create().
 *          Also updates idle task load/CPU Load.
 *
 *          This function is called, every "configLOAD_WINDOW_IN_MS" msecs within the IDLE task.
 *          It is important that idle task get to run atleast once every "configLOAD_WINDOW_IN_MS" msecs
 *          for the load statistics to be correct
 *          
 *          In case of FreeRTOS, FreeRTOSConfig.h enables update in idle
 *          and idle task calls this API.
 *          FreeRTOSConfig.h:-
 * 
 *              #define configLOAD_UPDATE_IN_IDLE   (1)
 *              #define configLOAD_WINDOW_IN_MS     (500)
 */
extern void LoadP_update(void);

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_LOADP_H_ */
