/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/RegisterIntr.h */

#ifndef _OSAL_REGISTER_INTR_H_
#define _OSAL_REGISTER_INTR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/HwiP.h" /* for HwiP_Handle */

typedef void (*Osal_IsrRoutine)(uintptr_t arg);

/*!
 *  @brief  Interrupt Configuration parameters for the corepac (c6x/a15/m5/a8/a9)
 */
typedef struct {
	char    *name;              /*!< Name of the instance for debugging purposes, could be set to NULL */
//	uint32_t corepacEventNum;    /*!< Event number going in to the DSP corepac */
	uint32_t intrNum;          /*!< Interrupt vector number for ARM corepac */
	Osal_IsrRoutine isrRoutine; /*!< The ISR routine to hook the corepacEventNum to */
	uintptr_t arg;              /*!< Argument to the ISR routine */
	uint32_t intrPrio;          /*!< Device specific priority for ARM corepac where a lower priority value
                                     indicates a higher priority, the priority range is corepac specific as
                                     listed below:
                                     - A8: 0-127
                                     - A9/A15/A53: 0-255
                                     - ARP32: 1(NMI), 2-13(INT4-15)
                                     - R5: 0-15
                                     - M4: None
                                     Note: For R5 Core, baremetal case, there is no device specific
                                           priority set - all interrupts are handled at the same priority
                                           So, this is not applicable for bare metal R5 Core interrupt config.
                                 */
	uint32_t intrType;           /*!< Set an interrupt's trigger sensitivity for ARM corepac as
                                      @ref OSAL_armGicTrigType_t. The applicable trigger types are
                                      corepac specific and listed below:
                                      - A53: Level and Edge Trigger
                                      - A15: Low/High Level and Rising/Falling Edge Trigger
                                      - A8/A9: Low/High Level and Rising/Falling/Both Edge Trigger
                                      - R5: Level Trigger and Pulse Trigger
                                  */
	uint32_t enableIntr;          /*!< When set to TRUE, interrupt is enabled after registration
                                       (@ref Osal_RegisterInterrupt) otherwise interrupt is disabled
                                   */
} OsalRegisterIntParams_corepac_t;

/*!
 *  @brief  Interrupt Configuration parameters
 *          This is the data structure used to configure the interrupts in the system.
 *   coreConfig: This part configures the interrupt for corepac such as c66x, arm, m4
 *   socMuxConfig: This part configures the interrupt path for any SOC level mux such as CIC/GIC or cross bar,
 *                 leading up to the corepac
 */
typedef struct {
	OsalRegisterIntParams_corepac_t corepacConfig;  /*!< configures the interrupt for corepac such as c66x, arm, m4 */
//	OsalRegisterIntParams_socmux_t  socMuxConfig;   /*!< configures the interrupt path for any SOC level mux such as
//                                                         CIC/GIC or cross bar, leading up to the corepac
//                                                     */
} OsalRegisterIntrParams_t;

/**
 *  \anchor OsalInterruptRetCode_e
 *  \name Osal Interrupt return Codes
 *  @{
 */
/*!
 *  @brief  Return error codes for  Osal Interrupt functions
 */
typedef int32_t OsalInterruptRetCode_e;
/*! Success return code */
#define  OSAL_INT_SUCCESS                       ((int32_t) 0)
/*! Invalid input parameters */
#define  OSAL_INT_ERR_INVALID_PARAMS            ((int32_t) -1)
/*! Error while registering the Hwi object */
#define  OSAL_INT_ERR_HWICREATE                 ((int32_t) -2)
/*! Error while registering the event combiner to the defaults */
#define  OSAL_INT_ERR_EVENTCOMBINER_REG         ((int32_t) -3)
/*! Failed to delete a Interrupt handle. */
#define  OSAL_INT_ERR_DELETE                    ((int32_t) -4)
/*! Unsupported function */
#define  OSAL_INT_UNSUPPORTED                   ((int32_t) -5)
/* @} */

/*!
 *  @brief  Function to initialize the interrupt registration configuration data structure
 *
 */
void Osal_RegisterInterrupt_initParams(OsalRegisterIntrParams_t *pIntrRegParams);

#if 0
/*!
 *  @brief  Function to register direct interrupt with parameters provided
 *
 */
OsalInterruptRetCode_e Osal_RegisterInterruptDirect(OsalRegisterIntrParams_t *interruptRegParams,
                                                    HwiP_DirectFxn isrFxn, HwiP_Handle *hwiPHandlePtr);
#endif

/*!
 *  @brief  Function to register interrupt with parameters provided
 *
 */
OsalInterruptRetCode_e Osal_RegisterInterrupt(OsalRegisterIntrParams_t *pIntrRegParams, HwiP_Handle *handle);

/*!
 *  @brief  Function to delete interrupt corresponding to an event number
 *
 */
OsalInterruptRetCode_e Osal_DeleteInterrupt(HwiP_Handle handle, uint32_t corepacEventNum /* Not used */);

/*!
 *  @brief  Function to enable the interrupt corresponding to an event number
 *
 */
void Osal_EnableInterrupt(uint32_t corepacEventNum /* Not used */, uint32_t intrNum);

/*!
 *  @brief  Function to disable the  interrupt corresponding to an event number
 *
 */
void Osal_DisableInterrupt(uint32_t corepacEventNum /* Not used */, uint32_t intrNum);

/*!
 *  @brief  Function to clear the interrupt corresponding to an event number
 *
 */
void Osal_ClearInterrupt(uint32_t corepacEventNum /* Not used */, uint32_t intrNum);

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_REGISTER_INTR_H_ */
