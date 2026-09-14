/*
 * Copyright (C) 2021-2022 Hikvision Auto Technology Co., Ltd.
 *   Author: wsl <wangshulin@hikvision.com>
 */

	.text

    /* Variables and functions. */
    .extern ullMaxAPIPriorityMask
    .extern pxCurrentTCB
    .extern vTaskSwitchContext
    .extern ullPortInterruptNesting
    .extern ullPortTaskHasFPUContext
    .extern ullCriticalNesting
    .extern ullPortYieldRequired
    .extern freertos_arm_gicv3_vectors
    .extern HwiP_irq_handler
    .extern HwiP_SError_Handler
    .extern HwiP_Sync_Abort_Handler

    .global FreeRTOS_SWI_Handler
    .global FreeRTOS_IRQ_Handler
    .global FreeRTOS_SError_Handler
    .global vPortRestoreTaskContext

.macro POP_ALL_CPU_REGS stackPtr
    LDP     X30, XZR, [\stackPtr], #0x10
    LDP     X28, X29, [\stackPtr], #0x10
    LDP     X26, X27, [\stackPtr], #0x10
    LDP     X24, X25, [\stackPtr], #0x10
    LDP     X22, X23, [\stackPtr], #0x10
    LDP     X20, X21, [\stackPtr], #0x10
    LDP     X18, X19, [\stackPtr], #0x10
    LDP     X16, X17, [\stackPtr], #0x10
    LDP     X14, X15, [\stackPtr], #0x10
    LDP     X12, X13, [\stackPtr], #0x10
    LDP     X10, X11, [\stackPtr], #0x10
    LDP     X8, X9, [\stackPtr], #0x10
    LDP     X6, X7, [\stackPtr], #0x10
    LDP     X4, X5, [\stackPtr], #0x10
    LDP     X2, X3, [\stackPtr], #0x10
    LDP     X0, X1, [\stackPtr], #0x10
.endm

.macro PUSH_ALL_INTERRUPT_FRAME_REGS stackPtr
    SUB     \stackPtr, \stackPtr, #32
    STP     X28, X29, [\stackPtr, #-16]!
    STP     X26, X27, [\stackPtr, #-16]!
    STP     X24, X25, [\stackPtr, #-16]!
    STP     X22, X23, [\stackPtr, #-16]!
    STP     X20, X21, [\stackPtr, #-16]!
    STP     X18, X19, [\stackPtr, #-16]!
    STP     X16, X17, [\stackPtr, #-16]!
    STP     X14, X15, [\stackPtr, #-16]!
    STP     X12, X13, [\stackPtr, #-16]!
    STP     X10, X11, [\stackPtr, #-16]!
    STP     X8, X9, [\stackPtr, #-16]!
    STP     X6, X7, [\stackPtr, #-16]!
    STP     X4, X5, [\stackPtr, #-16]!
    STP     X2, X3, [\stackPtr, #-16]!
    STP     X0, X1, [\stackPtr, #-16]!
    MRS     X0, SP_EL0
    MRS     X1, ELR_EL1
    MRS     X2, SPSR_EL1
    STP     X30, X0, [\stackPtr, #0xf0]
    STP     X1, X2, [\stackPtr, #0x100]
.endm

.macro POP_ALL_INTERRUPT_FRAME_REGS stackPtr
    LDP     X30, X0, [\stackPtr, #0xf0]
    LDP     X1, X2, [\stackPtr, #0x100]
    MSR     SP_EL0, X0
    MSR     ELR_EL1, X1
    MSR     SPSR_EL1, X2
    LDP     X0, X1, [\stackPtr], #0x10
    LDP     X2, X3, [\stackPtr], #0x10
    LDP     X4, X5, [\stackPtr], #0x10
    LDP     X6, X7, [\stackPtr], #0x10
    LDP     X8, X9, [\stackPtr], #0x10
    LDP     X10, X11, [\stackPtr], #0x10
    LDP     X12, X13, [\stackPtr], #0x10
    LDP     X14, X15, [\stackPtr], #0x10
    LDP     X16, X17, [\stackPtr], #0x10
    LDP     X18, X19, [\stackPtr], #0x10
    LDP     X20, X21, [\stackPtr], #0x10
    LDP     X22, X23, [\stackPtr], #0x10
    LDP     X24, X25, [\stackPtr], #0x10
    LDP     X26, X27, [\stackPtr], #0x10
    LDP     X28, X29, [\stackPtr], #0x10
    ADD     \stackPtr, \stackPtr, #32    
.endm

.macro PUSH_CALLER_SAVE_CPU_REGS stackPtr
	STP     X0, X1, [\stackPtr, #-16]!
    STP     X2, X3, [\stackPtr, #-16]!
    STP     X4, X5, [\stackPtr, #-16]!
    STP     X6, X7, [\stackPtr, #-16]!
    STP     X8, X9, [\stackPtr, #-16]!
    STP     X10, X11, [\stackPtr, #-16]!
    STP     X12, X13, [\stackPtr, #-16]!
    STP     X14, X15, [\stackPtr, #-16]!
    STP     X16, X17, [\stackPtr, #-16]!
    STP     X18, X19, [\stackPtr, #-16]!
    STP     X29, X30, [\stackPtr, #-16]!
.endm

.macro POP_CALLER_SAVE_CPU_REGS stackPtr
    LDP     X29, X30, [\stackPtr], #16
    LDP     X18, X19, [\stackPtr], #16
    LDP     X16, X17, [\stackPtr], #16
    LDP     X14, X15, [\stackPtr], #16
    LDP     X12, X13, [\stackPtr], #16
    LDP     X10, X11, [\stackPtr], #16
    LDP     X8, X9, [\stackPtr], #16
    LDP     X6, X7, [\stackPtr], #16
    LDP     X4, X5, [\stackPtr], #16
    LDP     X2, X3, [\stackPtr], #16
    LDP     X0, X1, [\stackPtr], #16    /* x0 & x1 saved by vector entry */
.endm

.macro PUSH_CALLER_SAVE_FPU_REGS stackPtr
    STP     q0, q1, [\stackPtr, #-32]!
    STP     q2, q3, [\stackPtr, #-32]!
    STP     q4, q5, [\stackPtr, #-32]!
    STP     q6, q7, [\stackPtr, #-32]!
    STP     q8, q9, [\stackPtr, #-32]!
    STP     q10, q11, [\stackPtr, #-32]!
    STP     q12, q13, [\stackPtr, #-32]!
    STP     q14, q15, [\stackPtr, #-32]!
    STP     q16, q17, [\stackPtr, #-32]!
    STP     q18, q19, [\stackPtr, #-32]!
    STP     q20, q21, [\stackPtr, #-32]!
    STP     q22, q23, [\stackPtr, #-32]!
    STP     q24, q25, [\stackPtr, #-32]!
    STP     q26, q27, [\stackPtr, #-32]!
    STP     q28, q29, [\stackPtr, #-32]!
    STP     q30, q31, [\stackPtr, #-32]!
.endm

.macro POP_CALLER_SAVE_FPU_REGS stackPtr
    LDP     q30, q31, [\stackPtr], #32
    LDP     q28, q29, [\stackPtr], #32
    LDP     q26, q27, [\stackPtr], #32
    LDP     q24, q25, [\stackPtr], #32
    LDP     q22, q23, [\stackPtr], #32
    LDP     q20, q21, [\stackPtr], #32
    LDP     q18, q19, [\stackPtr], #32
    LDP     q16, q17, [\stackPtr], #32
    LDP     q14, q15, [\stackPtr], #32
    LDP     q12, q13, [\stackPtr], #32
    LDP     q10, q11, [\stackPtr], #32
    LDP     q8, q9, [\stackPtr], #32
    LDP     q6, q7, [\stackPtr], #32
    LDP     q4, q5, [\stackPtr], #32
    LDP     q2, q3, [\stackPtr], #32
    LDP     q0, q1, [\stackPtr], #32
.endm

.macro PUSH_ALL_CPU_REGS stackPtr
    STP     X0, X1, [\stackPtr, #-16]!
    STP     X2, X3, [\stackPtr, #-16]!
    STP     X4, X5, [\stackPtr, #-16]!
    STP     X6, X7, [\stackPtr, #-16]!
    STP     X8, X9, [\stackPtr, #-16]!
    STP     X10, X11, [\stackPtr, #-16]!
    STP     X12, X13, [\stackPtr, #-16]!
    STP     X14, X15, [\stackPtr, #-16]!
    STP     X16, X17, [\stackPtr, #-16]!
    STP     X18, X19, [\stackPtr, #-16]!
    STP     X20, X21, [\stackPtr, #-16]!
    STP     X22, X23, [\stackPtr, #-16]!
    STP     X24, X25, [\stackPtr, #-16]!
    STP     X26, X27, [\stackPtr, #-16]!
    STP     X28, X29, [\stackPtr, #-16]!
    STP     X30, XZR, [\stackPtr, #-16]!
.endm

.macro portSAVE_CONTEXT

    /* Switch to use the EL0 stack pointer. */
    MSR     SPSEL, #0

    /* Save the entire context. */
    PUSH_ALL_CPU_REGS SP

    /* Save the SPSR. */
    MRS	    X3, SPSR_EL1
    MRS	    X2, ELR_EL1
    STP     X2, X3, [SP, #-0x10]!

    /* Save the critical section nesting depth. */
    LDR	    X0, ullCriticalNestingConst
    LDR	    X3, [X0]

    /* Save the FPU context indicator. */
    LDR	    X0, ullPortTaskHasFPUContextConst
    LDR	    X2, [X0]

    /* Save the FPU context, if any (32 128-bit registers). */
    CMP	    X2, #0
    B.EQ    1f
    PUSH_CALLER_SAVE_FPU_REGS SP

1:
    /* Store the critical nesting count and FPU context indicator. */
    STP     X2, X3, [SP, #-0x10]!

    LDR     X0, pxCurrentTCBConst
    LDR     X1, [X0]
    MOV     X0, SP   /* Move SP into X0 for saving. */
    STR     X0, [X1]

    /* Switch to use the ELx stack pointer. */
    MSR     SPSEL, #1

    .endm

; /**********************************************************************/

.macro portRESTORE_CONTEXT

    /* Switch to use the EL0 stack pointer. */
    MSR     SPSEL, #0

    /* Set the SP to point to the stack of the task being restored. */
    LDR	    X0, pxCurrentTCBConst
    LDR	    X1, [X0]
    LDR	    X0, [X1]
    MOV	    SP, X0

    LDP     X2, X3, [SP], #0x10  /* Critical nesting and FPU context. */

    /* Set the PMR register to be correct for the current critical nesting
	depth. */

    LDR	    X0, ullCriticalNestingConst /* X0 holds the address of ullCriticalNesting. */
    MOV	    X1, #255					/* X1 holds the unmask value. */
    CMP	    X3, #0
    B.EQ    2f
    LDR	    X6, ullMaxAPIPriorityMaskConst
    LDR	    X1, [X6]					/* X1 holds the mask value. */
2:
    MSR	    S3_0_C4_C6_0, X1
    DSB     SY							/* _RB_Barriers probably not required here. */
    ISB     SY
    STR	    X3, [X0]					/* Restore the task's critical nesting count. */

    /* Restore the FPU context indicator. */
    LDR     X0, ullPortTaskHasFPUContextConst
    STR	    X2, [X0]

    /* Restore the FPU context, if any. */
    CMP	    X2, #0
    B.EQ    1f
    POP_CALLER_SAVE_FPU_REGS SP
1:
    LDP     X2, X3, [SP], #0x10  /* SPSR and ELR. */

    /* Restore the SPSR. */
    MSR	    SPSR_EL1, X3
    /* Restore the ELR. */
    MSR	    ELR_EL1, X2

    POP_ALL_CPU_REGS SP

    /* Switch to use the ELx stack pointer.  _RB_ Might not be required. */
    MSR     SPSEL, #1

    ERET

    .endm

/******************************************************************************
 * FreeRTOS_SWI_Handler handler is used to perform a context switch.
 *****************************************************************************/
.align 8
.type FreeRTOS_SWI_Handler, %function
FreeRTOS_SWI_Handler:
    PUSH_ALL_INTERRUPT_FRAME_REGS SP

    MRS	    X0, ESR_EL1
    LSR	    X1, X0, #26
    CMP	    X1, #0x15 	/* 0x15 = SVC instruction. */
    B.NE    FreeRTOS_Sync_Abort

    POP_ALL_INTERRUPT_FRAME_REGS SP
    /* Save the context of the current task and select a new task to run. */
    portSAVE_CONTEXT
    BL     vTaskSwitchContext
    portRESTORE_CONTEXT

FreeRTOS_Sync_Abort:
    MOV	    X1, X0
    MOV     X0, SP
    BL HwiP_Sync_Abort_Handler
    POP_ALL_INTERRUPT_FRAME_REGS SP

    ERET

/******************************************************************************
 * vPortRestoreTaskContext is used to start the scheduler.
 *****************************************************************************/
.align 8
.type vPortRestoreTaskContext, %function
vPortRestoreTaskContext:
.set freertos_vector_base, freertos_arm_gicv3_vectors

    /* Install the FreeRTOS interrupt handlers. */
    LDR	    X1, =freertos_vector_base
    MSR	    VBAR_EL1, X1
    DSB	    SY
    ISB	    SY

    /* Start the first task. */
    portRESTORE_CONTEXT

/******************************************************************************
 * FreeRTOS_IRQ_Handler handles IRQ entry and exit.
 *****************************************************************************/
.align 8
.type FreeRTOS_IRQ_Handler, %function
FreeRTOS_IRQ_Handler:
    /* Save volatile registers. */
    PUSH_CALLER_SAVE_CPU_REGS SP
    PUSH_CALLER_SAVE_FPU_REGS SP

    /* Save the SPSR and ELR. */
    MRS	    X3, SPSR_EL1
    MRS	    X2, ELR_EL1
    STP     X2, X3, [SP, #-0x10]!

    /* Increment the interrupt nesting counter. */
    LDR	    X5, ullPortInterruptNestingConst
    LDR	    X1, [X5]	/* Old nesting count in X1. */
    ADD	    X6, X1, #1
    STR	    X6, [X5]	/* Address of nesting count variable in X5. */

    /* Maintain the interrupt nesting information across the function call. */
    STP     X1, X5, [SP, #-0x10]!

#if 0
    /* Read value from the interrupt acknowledge register, which is stored in W0
	for future parameter and interrupt clearing use. */

    MRS X0, S3_0_C12_C12_0

    /* ICCIAR in W0 as parameter. */
    /* Maintain the ICCIAR value across the function call. */
    STP	    X0, X1, [SP, #-0x10]!
#endif

    /* Call the C handler. */
    BL      HwiP_irq_handler
    /* Disable interrupts. */
    MSR     DAIFSET, #3

    DSB	    SY
    ISB	    SY

#if 0
    /* Restore the ICCIAR value. */
    LDP	    X0, X1, [SP], #0x10

    /* End IRQ processing by writing ICCIAR to the EOI register. */
    MSR  S3_0_C12_C12_1, X0
#endif

    /* Restore the critical nesting count. */
    LDP	    X1, X5, [SP], #0x10
    STR	    X1, [X5]

    /* Has interrupt nesting unwound? */
    CMP     X1, #0
    B.NE    Exit_IRQ_No_Context_Switch

    /* Is a context switch required? */
    LDR	    X0, ullPortYieldRequiredConst
    LDR	    X1, [X0]
    CMP	    X1, #0
    B.EQ    Exit_IRQ_No_Context_Switch

    /* Reset ullPortYieldRequired to 0. */
    MOV	    X2, #0
    STR	    X2, [X0]

    /* Restore volatile registers. */
    LDP     X4, X5, [SP], #0x10  /* SPSR and ELR. */
    MSR	    SPSR_EL1, X5
    MSR	    ELR_EL1, X4
    DSB	    SY
    ISB	    SY

    POP_CALLER_SAVE_FPU_REGS SP
    POP_CALLER_SAVE_CPU_REGS SP

    /* Save the context of the current task and select a new task to run. */
    portSAVE_CONTEXT
    BL vTaskSwitchContext
    portRESTORE_CONTEXT

Exit_IRQ_No_Context_Switch:
    /* Restore volatile registers. */
    LDP     X4, X5, [SP], #0x10  /* SPSR and ELR. */
    MSR	    SPSR_EL1, X5
    MSR	    ELR_EL1, X4
    DSB	    SY
    ISB	    SY

    POP_CALLER_SAVE_FPU_REGS SP
    POP_CALLER_SAVE_CPU_REGS SP

    ERET

/******************************************************************************
 * FreeRTOS_SError_Handler handles SError/vSError exception.
 *****************************************************************************/
.align 8
.type FreeRTOS_SError_Handler, %function
FreeRTOS_SError_Handler:
    PUSH_ALL_INTERRUPT_FRAME_REGS SP
    MOV     X0, SP
    MRS	    X1, ESR_EL1
    BL HwiP_SError_Handler
    POP_ALL_INTERRUPT_FRAME_REGS SP

    ERET

.align 8
pxCurrentTCBConst: .dword pxCurrentTCB
ullCriticalNestingConst: .dword ullCriticalNesting
ullPortTaskHasFPUContextConst: .dword ullPortTaskHasFPUContext
ullMaxAPIPriorityMaskConst: .dword ullMaxAPIPriorityMask
vApplicationIRQHandlerConst: .dword HwiP_irq_handler
ullPortInterruptNestingConst: .dword ullPortInterruptNesting
ullPortYieldRequiredConst: .dword ullPortYieldRequired

    .end
