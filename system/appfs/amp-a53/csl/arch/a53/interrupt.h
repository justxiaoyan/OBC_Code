/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_INTERRUPT_H_
#define _CSL_INTERRUPT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * External interrupt from the system
 * ID00-ID15:  SGI (Software generated interrupts)
 * ID16-ID31:  PPI (Private Peripheral interrupts)
 * ID31-ID960: SPI (Shared peripheral interrupts)
 */
#define NUM_INTERRUPTS_EXTERNAL     (960U)
#define NUM_INTERRUPTS              (1024U)

/* ========================================================================== */
/*                          Internal typedef Declarations                     */
/* ========================================================================== */
typedef void (*IntrFuncPtr)(void *handle);

/**
 * \brief    Registers an interrupt Handler in the interrupt vector table for
 *           system interrupts.
 *
 * \param    intrNum - Interrupt Number
 * \param    fnHandler - Function pointer to the ISR
 * \param    fun_arg - Argument to the ISR
 *
 * Note: When the interrupt occurs for the sytem interrupt number indicated,
 * the control goes to the ISR given as the parameter.
 *
 * \return      None.
 **/
void Intc_IntRegister(uint16_t intrNum, IntrFuncPtr fptr, void *fun_arg);

/**
 * \brief   Unregisters an interrupt
 *
 * \param   intrNum - Interrupt Number
 *
 * Note: Once an interrupt is unregistered it will enter infinite loop once
 * an interrupt occurs
 *
 * \return      None.
 **/
void Intc_IntUnregister(uint16_t intrNum);

/**
 * \brief   This API is used to initialize the interrupt controller. This API
 *          shall be called before using the interrupt controller.
 *
 * \param   coreId - core ID
 *
 * \return  None.
 *
 **/
void Intc_Init(uint8_t corId);

/**
 * \brief     This API is called to check if interrupt controller
 *            is initialized via INTC CSL APIs.
 *
 * \param     None.
 *
 * \return    True if intc is initialized via CSL.
 *
 **/
bool Intc_isInitialized(void);

/**
 * \brief     This API is called to Assign Level interrupt
 *            to the SPI interrupt number.
 *
 * \param     None.
 *
 * \return    True if operation is successful
 *
 **/
int32_t Intc_IntAssignLevelIntType(uint16_t intrNum);

/**
 * \brief     This API is called to Assign Edge interrupt
 *            to the SPI interrupt number.
 *
 * \param     None.
 *
 * \return    True if operation is successful
 *
 **/
int32_t Intc_IntAssignEdgeIntType(uint16_t intrNum);

/**
 * \brief   This API assigns a priority to an interrupt and routes it to
 *          either IRQ or to FIQ. Priority 0 is the highest priority level
 *          Among the host interrupts, FIQ has more priority than IRQ.
 *
 * \param   intrNum  - Interrupt number
 * \param   priority - Interrupt priority level
 *
 * \return    True if operation is successful
 *
 **/
int32_t Intc_IntPrioritySet(uint16_t intrNum, uint16_t priority);

/**
 * \brief   This API enables the system interrupt in INTC. However, for
 *          the interrupt generation, make sure that the interrupt is
 *          enabled at the peripheral level also.
 *
 * \param   intrNum  - Interrupt number
 *
 * \return  Previous state of PSR.
 *
 **/
uint32_t Intc_SystemEnable(uint16_t intrNum);

/**
 * \brief   This API disables the system interrupt in INTC.
 *
 * \param   intrNum  - Interrupt number
 *
 * \return  Previous state of PSR.
 *
 **/
uint32_t Intc_SystemDisable(uint16_t intrNum);

/**
 * \brief     This API is called  When the interrupt occurs ,
 *            the control goes to the ISR which is registered
 *            using Intc_IntRegister else will result in infinite loop.
 *
 * \param     None.
 *
 * \return    None.
 *
 **/
void IntCommonHandler(void);

/**
 * \brief     This API is called to set the interrupt affinity
 *            to the SPI interrupt number.
 *
 * \param     intrNum  - Interrupt number.
 *
 * \return    True if operation is successful
 *
 **/
int32_t Intc_IntSetAffinity(uint16_t intrNum);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_INTERRUPT_H_ */
