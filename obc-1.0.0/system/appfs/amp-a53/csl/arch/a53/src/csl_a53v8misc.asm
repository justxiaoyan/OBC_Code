/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/csl/arch/a53/src/csl_a53v8misc.asm. */

    .section arm64misc, "ax", %progbits

/*
 *  void CSL_a53Exit(void)
 *  This is a baremetal exit code
 *  After the baremetal application is complete, the control comes
 *  here and waits until a system reset/power cycle
 */
    .global CSL_a53Exit
    .type CSL_a53Exit  , %function
CSL_a53Exit:
     B CSL_a53Exit

    .global CSL_a53v8DsbSy
    .type CSL_a53v8DsbSy  , %function
CSL_a53v8DsbSy:
    DSB SY    ; /* Full System DSB Operation */
    BR  X30       ; /*Return to caller */

    .global CSL_a53v8Isb
    .type CSL_a53v8Isb, %function
CSL_a53v8Isb:
    ISB
    BR  X30

    .global CSL_a53v8GetCpuId
    .type   CSL_a53v8GetCpuId, %function
CSL_a53v8GetCpuId:
    mrs     x0, mpidr_el1        /* Returns are in x0(r0) */
    ret                          /* Return to caller */


    .global CSL_a53v8WriteTtbr0El3
    .type   CSL_a53v8WriteTtbr0El3, %function
CSL_a53v8WriteTtbr0El3:
    msr     ttbr0_el3,x0
    ret

    .global CSL_a53v8WriteTtbr0El2
    .type  CSL_a53v8WriteTtbr0El2 , %function
CSL_a53v8WriteTtbr0El2:
    msr     ttbr0_el2,x0
    ret

    .global CSL_a53v8WriteTtbr0El1
    .type   CSL_a53v8WriteTtbr0El1, %function
CSL_a53v8WriteTtbr0El1:
    msr     ttbr0_el1,x0
    ret

    /* Exception Level Transition Functions */


    .global CSL_a53v8SwitchLevelEl3toEl2
    .type   CSL_a53v8SwitchLevelEl3toEl2, %function
CSL_a53v8SwitchLevelEl3toEl2:
    msr     SCTLR_EL2, xzr          /*Initialize SCTRL_EL2 to safe values */
    mov     x1, #0x401              /* EL2 is AArch64 */
    msr     SCR_EL3, x1             /* SCR_EL3.RW = .NS = 0b1 */
    mov     x1, #0b01001            /* Setup SPRS_EL3 to be "AArch64 with SP_EL2h" */
    msr     SPSR_EL3, x1
    msr     ELR_EL3, x0             /* x0 is the entry point and argument to the function */
    eret

    .global CSL_a53v8SwitchLevelEl3toNsEl1
    .type   CSL_a53v8SwitchLevelEl3toNsEl1, %function
CSL_a53v8SwitchLevelEl3toNsEl1:
    msr     SCTLR_EL1, xzr          /*Initialize SCTRL_EL2 to safe values */
    mov     x1, #0x501              /* EL2 is AArch64, enable HVC[8] */
    msr     SCR_EL3, x1             /* SCR_EL3.RW = .NS = 0b1 */
    mov     x1, #0x80000000         /* EL2 is AArch64 */
    msr     HCR_EL2, x1             /* SCR_EL3.RW = .NS = 0b1 */
    mov     x1, #0b00101            /* Setup SPRS_EL3 to be "AArch64 with SP_EL1h" */
    msr     SPSR_EL3, x1
    msr     ELR_EL3, x0             /* x0 is the entry point and argument to the function */
    eret

    .global CSL_a53v8SwitchLevelEl3toSEl1
    .type   CSL_a53v8SwitchLevelEl3toSEl1, %function
CSL_a53v8SwitchLevelEl3toSEl1:
    msr     SCTLR_EL1, xzr          /*Initialize SCTRL_EL2 to safe values */
    mov     x1, #0x500              /* EL2 is AArch64, enable HVC[8] */
    msr     SCR_EL3, x1             /* SCR_EL3.RW = .NS = 0b1 */
    mov     x1, #0b00101            /* Setup SPRS_EL3 to be "AArch64 with SP_EL1h" */
    msr     SPSR_EL3, x1
    msr     ELR_EL3, x0             /* x0 is the entry point and argument to the function */
    eret

    /*========================================================================= */
    /* change_el3_to_nsel0 */
    /* */
    /* Transition from EL3 to Non-Secure EL0 */
    /* */
    /*========================================================================= */
    .global CSL_a53v8SwitchLevelEl3toNsEl0
    .type   CSL_a53v8SwitchLevelEl3toNsEl0, %function
CSL_a53v8SwitchLevelEl3toNsEl0:
    msr     SCTLR_EL1, xzr          /*Initialize SCTRL_EL2 to safe values */
    mov     x1, #0x401              /* EL2 is AArch64 */
    msr     SCR_EL3, x1             /* SCR_EL3.RW = .NS = 0b1 */
    mov     x1, #0x80000000         /* EL1 is AArch64 */
    msr     HCR_EL2, x1             /* HCR_EL3.RW = 0b1 */
    mov     x1, #0b0000             /* Setup SPRS_EL3 to be "AArch64 with SP_EL0t" */
    msr     SPSR_EL3, x1
    msr     ELR_EL3, x0             /* x0 is the entry point and argument to the function */
    eret

    .global CSL_a53v8SwitchLevelEl3toSEl0
    .type   CSL_a53v8SwitchLevelEl3toSEl0, %function
CSL_a53v8SwitchLevelEl3toSEl0:
    msr     SCTLR_EL1, xzr          /*Initialize SCTRL_EL2 to safe values */
    mov     x1, #0x400              /* EL2 is AArch64 */
    msr     SCR_EL3, x1             /* SCR_EL3.RW = .NS = 0b1 */
    mov     x1, #0x80000000         /* EL1 is AArch64 */
    msr     HCR_EL2, x1             /* HCR_EL3.RW = 0b1 */
    mov     x1, #0b0000             /* Setup SPRS_EL3 to be "AArch64 with SP_EL0t" */
    msr     SPSR_EL3, x1
    msr     ELR_EL3, x0             /* x0 is the entry point and argument to the function */
    eret

    .global CSL_a53v8SwitchLeveltoEl3
    .type   CSL_a53v8SwitchLeveltoEl3, %function
CSL_a53v8SwitchLeveltoEl3:
    smc     #0                      /* Secure Monitor Call causes exception at EL3 */
    ret

    .global CSL_a53v8SwitchLevelEl0toEl3
    .type   CSL_a53v8SwitchLevelEl0toEl3, %function
CSL_a53v8SwitchLevelEl0toEl3:
    svc     #0                      /* Supervisor Call causes exception at EL1 */
    smc     #0                      /* Secure Monitor Call causes exception at EL3 */
    ret

    .global CSL_a53v8SwitchLeveltoEl2
    .type  CSL_a53v8SwitchLeveltoEl2 , %function
CSL_a53v8SwitchLeveltoEl2:
    hvc     #0
    ret

    .global CSL_a53v8ArmGicv3HwiDisable
    .type   CSL_a53v8ArmGicv3HwiDisable, %function
CSL_a53v8ArmGicv3HwiDisable:
    mrs x0, daif
    msr daifset, #2
    ret

    .global CSL_a53v8ArmGicv3HwiEnable
    .type   CSL_a53v8ArmGicv3HwiEnable, %function
CSL_a53v8ArmGicv3HwiEnable:
    mrs x0, daif
    msr daifclr, #2
    ret

    .global CSL_a53v8ArmGicv3HwiRestore
    .type   CSL_a53v8ArmGicv3HwiRestore, %function
CSL_a53v8ArmGicv3HwiRestore:
    and x2, x0, #128
    mrs x1, daif
    bic x1, x1, #128
    orr x1, x1, x2
    msr daif, x1
    ret

    .end
