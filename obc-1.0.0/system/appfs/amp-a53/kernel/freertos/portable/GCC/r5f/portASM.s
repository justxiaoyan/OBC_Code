/*
@  FreeRTOS Kernel V10.4.1
;  Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
; 
;  Permission is hereby granted, free of charge, to any person obtaining a copy of
;  this software and associated documentation files (the "Software"), to deal in
;  the Software without restriction, including without limitation the rights to
;  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
;  the Software, and to permit persons to whom the Software is furnished to do so,
;  subject to the following conditions:
; 
;  The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
; 
;  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
;  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
;  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
;  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
;  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
; 
;  https://www.FreeRTOS.org
;  https://github.com/FreeRTOS
; 
;  1 tab == 4 spaces!  */

/*-----------------------------------------------------------*/
 	.section .startup_code
        .syntax unified
        .cpu cortex-r5
        .arm     
        .extern ulPortYieldRequired
        .extern ulCriticalNesting
        .extern ulPortTaskHasFPUContext
        .extern vTaskSwitchContext
        .extern ulPortInterruptNesting
        .extern irq_handler_c
        .extern data_abort_handler_c
		.extern fiq_handler_c
/*-----------------------------------------------------------*/


/*-------------------------------------------------------------------------------*/
		.macro portSAVE_CONTEXT 
        @ Save the LR and SPSR onto the system mode stack before switching to
        @ system mode to save the remaining system mode registers.
        SRSDB   sp!, #0x1F
        CPS     #0x1F
        PUSH    {R0-R12, R14}

        @ Push the critical nesting count.
        LDR     R2, ulCriticalNestingConst
        LDR     R1, [R2]
        PUSH    {R1}

        @  Does the task have a floating point context that needs saving?  If
        @  ulPortTaskHasFPUContext is 0 then no.
        LDR     R2, ulPortTaskHasFPUContextConst
        LDR     R3, [R2]
        CMP     R3, #0

        @  Save the floating point context, if any. 
        FMRXNE  R1,  FPSCR
        VPUSHNE {D0-D15}
        @  VPUSHNE      {D16-D31} 
        PUSHNE  {R1}

        @  Save ulPortTaskHasFPUContext itself.
        PUSH    {R3}

        @  Save the stack pointer in the TCB.
        LDR     R0, pxCurrentTCBConst
        LDR     R1, [R0]
        STR     SP, [R1]

        .endm
/*-------------------------------------------------------------------------------*/
		.macro portRESTORE_CONTEXT 
        @  Set the SP to point to the stack of the task being restored.
        LDR     R0, pxCurrentTCBConst
        LDR     R1, [R0]
        LDR     SP, [R1]

        @  Is there a floating point context to restore?  If the restored
        @  ulPortTaskHasFPUContext is zero then no.
        LDR     R0, ulPortTaskHasFPUContextConst
        POP     {R1}
        STR     R1, [R0]
        CMP     R1, #0

        @  Restore the floating point context, if any.
        POPNE   {R0}
        @  VPOPNE       {D16-D31} 
        VPOPNE  {D0-D15}
        VMSRNE  FPSCR, R0

        @  Restore the critical section nesting depth.
        LDR     R0, ulCriticalNestingConst
        POP     {R1}
        STR     R1, [R0]

        @ reset local monitor, this is needed for atomics which use LDREX, STREX to work properly
        CLREX

        @  Restore all system mode registers other than the SP (which is already
        @  being used).
        POP     {R0-R12, R14}

        @  Return to the task code, loading CPSR on the way.
        RFEIA   sp!

        .endm

/*-------------------------------------------------------------------------------*/        
@  FUNCTION DEF: void vPortRestoreTaskContext(void) 
	.weak vPortRestoreTaskContext
        .type vPortRestoreTaskContext, %function
       
vPortRestoreTaskContext: 
        @  Switch to system mode.
        CPS     #0x1F
        portRESTORE_CONTEXT
/*-------------------------------------------------------------------------------*/        

@  FUNCTION DEF: void swi_handler(void)    
	.weak swi_handler
        .type swi_handler, %function    
       
swi_handler: 
        @  Save the context of the current task and select a new task to run.
	portSAVE_CONTEXT
	LDR     R0, vTaskSwitchContextConst
        BLX     R0
        portRESTORE_CONTEXT
/*-------------------------------------------------------------------------------*/        


@  FUNCTION DEF: void irq_handler(void)      
	.weak irq_handler
        .type irq_handler, %function    
irq_handler: 
	@  Return to the interrupted instruction.
	SUB		lr, lr, #4

	@  Push the return address and SPSR.
	PUSH	{lr}
	MRS	lr, SPSR
	PUSH	{lr}

	@  Change to supervisor mode to allow reentry.
	@  CPS	SVC_MODE

	@  Push used registers.
	PUSH	{r0-r4, r12}

	@  Increment nesting count.  r3 holds the address of ulPortInterruptNesting
	@  for future use.  r1 holds the original ulPortInterruptNesting value for
	@  future use. 
	LDR	r3, ulPortInterruptNestingConst
	LDR	r1, [r3]
	ADD	r4, r1, #1
	STR	r4, [r3]

	@  Save the floating point context
	FMRX  R0, FPSCR
	VPUSH {D0-D15}
	@ VPUSH {D16-D31} 
	PUSH  {R0}

	@  Ensure bit 2 of the stack pointer is clear.  r2 holds the bit 2 value for
	@  future use.  _RB_ Is this ever needed provided the start of the stack is
	@  alligned on an 8-byte boundary? 
	MOV	r2, sp
	AND	r2, r2, #4
	SUB	sp, sp, r2

	@  Call the interrupt handler.
	PUSH	{r0-r4, lr}
	LDR	r1, vApplicationIRQHandlerConst
	BLX	r1
	POP	{r0-r4, lr}
	ADD	sp, sp, r2

        @  disable IRQ
	CPSID	i
	DSB
	ISB

	@  Restore the floating point context
	POP   {R0}
	@ VPOP {D16-D31}
	VPOP  {D0-D15}
	VMSR  FPSCR, R0

	@  Restore the old nesting count.
	STR	r1, [r3]

	@  A context switch is never performed if the nesting count is not 0.
	CMP	r1, #0
	BNE	exit_without_switch

	@  Did the interrupt request a context switch?  r1 holds the address of
	@  ulPortYieldRequired and r0 the value of ulPortYieldRequired for future
	@  use. 
	LDR	r1, ulPortYieldRequiredAddr
	LDR	r0, [r1]
	CMP	r0, #0
	BNE	switch_before_exit
        

exit_without_switch:
	@  No context switch.  Restore used registers, LR_irq and SPSR before
	@  returning. 
	POP	{r0-r4, r12}
	@  CPS	IRQ_MODE
	POP	{LR}
	MSR	SPSR_cxsf, LR
	POP	{LR}
	MOVS	PC, LR

switch_before_exit:
	@  A context swtich is to be performed.  Clear the context switch pending
	@  flag.
	MOV	r0, #0
	STR	r0, [r1]

	@  Restore used registers, LR-irq and SPSR before saving the context
	@  to the task stack.
	POP	{r0-r4, r12}
	@  CPS	IRQ_MODE
	POP	{LR}
	MSR	SPSR_cxsf, LR
	POP	{LR}
	portSAVE_CONTEXT

	@  Call the function that selects the new task to execute.
	@  vTaskSwitchContext() if vTaskSwitchContext() uses LDRD or STRD
	@  instructions, or 8 byte aligned stack allocated data.  LR does not need
	@  saving as a new LR will be loaded by portRESTORE_CONTEXT anyway. 
	LDR	R0, vTaskSwitchContextConst
	BLX	R0

	@  Restore the context of, and branch to, the task selected to execute
	@  next. 
	portRESTORE_CONTEXT


/*-------------------------------------------------------------------------------*/        


@  FUNCTION DEF: void irq_handler(void)      
	.weak fiq_handler
        .type fiq_handler, %function    
fiq_handler: 
	@  Return to the interrupted instruction.
	SUB		lr, lr, #4

	@  Push the return address and SPSR.
	PUSH	{lr}
	MRS	lr, SPSR
	PUSH	{lr}

	@  Change to supervisor mode to allow reentry.
	@  CPS	SVC_MODE

	@  Push used registers.
	PUSH	{r0-r4, r12}

	
	@  Save the floating point context
	FMRX  R0, FPSCR
	VPUSH {D0-D15}
	@ VPUSH {D16-D31} 
	PUSH  {R0}

	@  Ensure bit 2 of the stack pointer is clear.  r2 holds the bit 2 value for
	@  future use.  _RB_ Is this ever needed provided the start of the stack is
	@  alligned on an 8-byte boundary? 
	MOV	r2, sp
	AND	r2, r2, #4
	SUB	sp, sp, r2

	@  Call the interrupt handler.
	PUSH	{r0-r4, lr}
	LDR	r1, vApplicationFIQHandlerConst
	BLX	r1
	POP	{r0-r4, lr}
	ADD	sp, sp, r2

        @  disable IRQ
	CPSID	f
	DSB
	ISB

	@  Restore the floating point context
	POP   {R0}
	@ VPOP {D16-D31}
	VPOP  {D0-D15}
	VMSR  FPSCR, R0
	

	@  No context switch.  Restore used registers, LR_fiq and SPSR before
	@  returning. 
	POP	{r0-r4, r12}
	@  CPS	FIQ_MODE
	POP	{LR}
	MSR	SPSR_cxsf, LR
	POP	{LR}
	MOVS	PC, LR


@  FUNCTION DEF: void data_abort_handler(void)
       	.weak data_abort_handler
        .type data_abort_handler, %function
data_abort_handler:
	@  Return to the instruction following the interrupted.
	SUB	lr, lr, #4

	@  Push the return address and SPSR.
	PUSH	{lr}
	MRS	lr, SPSR
	PUSH	{lr}

	@  Push used registers.
	PUSH	{r0-r4, r12}

	MOV	r0, sp

	@  Call the interrupt handler.
	LDR	r1, vApplicationDataAbortHandlerConst
	BLX	r1

	@  Restore used registers, LR and SPSR before  returning.
	POP	{r0-r4, r12}
	POP	{LR}
	MSR	SPSR_cxsf, LR
	POP	{LR}
	MOVS	PC, LR

ulPortYieldRequiredAddr: .word ulPortYieldRequired
pxCurrentTCBConst: .word pxCurrentTCB
ulCriticalNestingConst: .word ulCriticalNesting
ulPortTaskHasFPUContextConst: .word ulPortTaskHasFPUContext
vTaskSwitchContextConst: .word vTaskSwitchContext
ulPortInterruptNestingConst: .word ulPortInterruptNesting
vApplicationIRQHandlerConst: .word irq_handler_c
vApplicationDataAbortHandlerConst: .word data_abort_handler_c
vApplicationFIQHandlerConst: .word fiq_handler_c
SYS_MODE:   .word     0x1F
SVC_MODE:   .word     0x13 
IRQ_MODE:   .word     0x12 

