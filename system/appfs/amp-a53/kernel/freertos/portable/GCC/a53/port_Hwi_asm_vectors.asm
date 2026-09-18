/*
 * Copyright (C) 2021-2022 Hikvision Auto Technology Co., Ltd.
 *   Author: wsl <wangshulin@hikvision.com>
 */

/*
 * ======== port_Hwi_asm_vectors.asm ========
 */
    .global freertos_arm_gicv3_vectors
	
    .section .freertosvectors, "ax"
    .align  12
freertos_arm_gicv3_vectors:

/**************************************************************************
 * Vector table to use when FreeRTOS is running.
 *************************************************************************/
.set    FREERTOS_VBAR,  (freertos_arm_gicv3_vectors)

/**************************************************************************
 * Exception from currentEL, using SP0
 *************************************************************************/
.org (FREERTOS_VBAR)
    b   FreeRTOS_SWI_Handler

.org (FREERTOS_VBAR + 0x80)
    b   FreeRTOS_IRQ_Handler

.org (FREERTOS_VBAR + 0x100)
    b   FreeRTOS_IRQ_Handler

.org (FREERTOS_VBAR + 0x180)
    b   FreeRTOS_SError_Handler

/**************************************************************************
 * Exception from currentEL, using SPx
 *************************************************************************/
.org (FREERTOS_VBAR + 0x200)
    b   FreeRTOS_SWI_Handler

.org (FREERTOS_VBAR + 0x280)
    b   FreeRTOS_IRQ_Handler

.org (FREERTOS_VBAR + 0x300)
    b   FreeRTOS_IRQ_Handler

.org (FREERTOS_VBAR + 0x380)
    b   FreeRTOS_SError_Handler

/**************************************************************************
 * Exception from lowerEL, all lowerEL using Aarch64
 *************************************************************************/
.org (FREERTOS_VBAR + 0x400)
    b   .

.org (FREERTOS_VBAR + 0x480)
    b   .

.org (FREERTOS_VBAR + 0x500)
    b   .

.org (FREERTOS_VBAR + 0x580)
    b   .

/**************************************************************************
 * Exception from lowerEL, all lowerEL using Aarch32
 *************************************************************************/
.org (FREERTOS_VBAR + 0x600)
    b   .

.org (FREERTOS_VBAR + 0x680)
    b   .

.org (FREERTOS_VBAR + 0x700)
    b   .

.org (FREERTOS_VBAR + 0x780)
    b   .

    .end
