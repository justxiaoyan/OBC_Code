/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/src/nonos/RegisterIntr_nonos.c */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "osal/src/nonos/Nonos_config.h"
#include "osal/RegisterIntr.h"

/*
 * Purpose:     Initializes the interrupt registration config structure
 * Description: Initializes the interrupt registration config structure prior to
 *              invoking the Osal_registerIntr API
 */
void Osal_RegisterInterrupt_initParams(OsalRegisterIntrParams_t *pIntrRegParams)
{
	if (pIntrRegParams != NULL_PTR) {
		/* Corepac config */
		pIntrRegParams->corepacConfig.name = (char *)NULL_PTR;
//		pIntrRegParams->corepacConfig.corepacEventNum = -1;
		pIntrRegParams->corepacConfig.intrNum = 0; /* RReconfiguration is required */
		pIntrRegParams->corepacConfig.isrRoutine = (void (*)(uintptr_t arg))NULL_PTR;
		pIntrRegParams->corepacConfig.arg = (uintptr_t)NULL_PTR;
		pIntrRegParams->corepacConfig.intrPrio = 0x20U; /* Default */
		pIntrRegParams->corepacConfig.enableIntr = 1U;
		pIntrRegParams->corepacConfig.intrType = (uint32_t)OSAL_GIC_INTR_TYPE_LEVEL; /* Interrupt level triggered */
//		/* SOC Mux Config */
//		pIntrRegParams->socMuxConfig.muxInParams = (MuxIntcP_inParams *)NULL_PTR;
//		pIntrRegParams->socMuxConfig.muxOutParams = (MuxIntcP_outParams *)NULL_PTR;
	}
}

/*
 * Purpose:     Registers the interrupt for the event with the params provided
 * Description: Registers the event with the event combiner/Interrupt Vector.
 *
 * Returns:     The Hwi Handle and the return codes defined in OsalInterruptRetCode_e
 */
OsalInterruptRetCode_e Osal_RegisterInterrupt(OsalRegisterIntrParams_t *pIntrRegParams, HwiP_Handle *handle)
{
	OsalInterruptRetCode_e ret = OSAL_INT_SUCCESS;

	if ((pIntrRegParams->corepacConfig.isrRoutine == (void (*)(uintptr_t arg))NULL_PTR) /* ||
		(pIntrRegParams->corepacConfig.corepacEventNum < 0) */) {
		ret = OSAL_INT_ERR_INVALID_PARAMS;
	}

	if (ret == OSAL_INT_SUCCESS) {
		HwiP_Handle handle_ = NULL_PTR;
		HwiP_Params params;

		HwiP_Params_init(&params);

		params.name = pIntrRegParams->corepacConfig.name;
		params.arg  = pIntrRegParams->corepacConfig.arg;
		params.intrPrio = pIntrRegParams->corepacConfig.intrPrio;
//		params.eventId = pIntrRegParams->corepacConfig.corepacEventNum; /* Not used */
		params.enableIntr = pIntrRegParams->corepacConfig.enableIntr;
		params.intrType = pIntrRegParams->corepacConfig.intrType;

		handle_ = HwiP_create(pIntrRegParams->corepacConfig.intrNum, pIntrRegParams->corepacConfig.isrRoutine, &params);
		if (handle_ == NULL_PTR) {
			ret = OSAL_INT_ERR_HWICREATE;
		}
		*handle = handle_;
	}

	return ret;
}

/*
 * Purpose:     Deletes/Unregisters an interrupt
 * Description: Deletes/Unregisters an interrupt associated with an event number(0-127).
 *
 * Returns:     OSAL_INT_SUCCESS if successful,OSAL_INT_ERR_DELETE if failed
 */
OsalInterruptRetCode_e Osal_DeleteInterrupt(HwiP_Handle handle, uint32_t corepacEventNum)
{
	HwiP_Status status;
	OsalInterruptRetCode_e ret = OSAL_INT_SUCCESS;

	(void)corepacEventNum;

	status = HwiP_delete(handle);
	if (status != HwiP_OK) {
		ret = OSAL_INT_ERR_DELETE;
	}

	return ret;
}

/*
 * Purpose:     Enables an event/interrupt
 * Description: Enables the interrupt associated with an event number(0-127).
 *              In case of the event combiner, the corresponding event is
 *              enabled within the Event Combiner.
 *
 * Returns:     None
 */
void Osal_EnableInterrupt(uint32_t corepacEventNum, uint32_t intrNum)
{
	(void)corepacEventNum;
	HwiP_enableInterrupt(intrNum);
}

/*
 * Purpose:     Disables an event/interrupt
 * Description: Disables the interrupt associated with an event number(0-127).
 *              In case of the event combiner, the corresponding event is
 *              disabled within the Event Combiner.
 *
 * Returns:     None
 */
void Osal_DisableInterrupt(uint32_t corepacEventNum, uint32_t intrNum)
{
	(void)corepacEventNum;
	HwiP_disableInterrupt(intrNum);
}

/*
 * Purpose:     Clears a pending interrupt
 * Description: Clears a pending interrupt associated with an event number(0-127).
 *
 * Returns:     None
 */
void Osal_ClearInterrupt(uint32_t corepacEventNum, uint32_t intrNum)
{
	(void)corepacEventNum;
	HwiP_clearInterrupt(intrNum);
}
