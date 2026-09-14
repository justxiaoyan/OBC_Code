/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/csl/arch/a53/src/interrupt.c */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "csl/csl_gic.h"
#include "csl/arch/csl_arch.h"

/* Refer to "ARM® Generic Interrupt Controller Architecture Specification (version 3.0 and version 4.0)" */

#define CSL_writeSystemReg(regName, val) \
    {                                                                \
        __asm__ __volatile__ (                                       \
            "msr " #regName ", %0"                                   \
            :: "r" (val)                                             \
        );                                                           \
    }

#define CSL_readSystemReg(regName, val)  \
    {                                                                \
        __asm__ __volatile__ (                                       \
            "mrs %0, " #regName                                      \
            : "=r" (val)                                             \
        );                                                           \
    }

static uint64_t gCslA55InitControllerStatus = 0;
static IntrFuncPtr gFnRAMVectors[NUM_INTERRUPTS];
static void *gArgArray[NUM_INTERRUPTS] = {0};

/**
 *
 * The Default Interrupt Handler.
 *
 * This is the default interrupt handler for all interrupts.  It simply loops
 * forever so that the system state is preserved for observation by a
 * debugger.  Since interrupts should be disabled before unregistering the
 * corresponding handler, this should never be called.
 *
 *
 **/
void IntDefaultHandler(void *dummy)
{
	/* Go into an infinite loop.*/
	volatile uint32_t loop = 1U;
	while (1U == loop) {
		;
	}
}

/**
 *
 * The Common Interrupt Handler.
 *
 * This is the common interrupt handler for all interrupts.
 *
 *
 **/
void IntCommonHandler(void)
{
	uint64_t intrNum;

	/* Acknowledge Interrupt */
	CSL_readSystemReg(s3_0_c12_c12_0, intrNum); /* icc_iar1_el1 */

	/* Ignore spurious ints */
	if ((intrNum >= 1020) && (intrNum <= 1023))
		return;

	/* Call user ISR function in system mode and then return back to IRQ mode */
	(gFnRAMVectors[intrNum])(gArgArray[intrNum]);

	/* Signal End of Interrupt */
	CSL_writeSystemReg(s3_0_c12_c12_1, intrNum); /* icc_eoir1_el1 */
}

void Intc_IntRegister(uint16_t intrNum, IntrFuncPtr fptr, void *fun_arg)
{
    /* Assign ISR */
    gFnRAMVectors[intrNum] = fptr;
    gArgArray[intrNum] = fun_arg;
    return;
}

int32_t Intc_IntAssignLevelIntType(uint16_t intrNum)
{
	return CSL_gicAssignLevelIntrType((uintptr_t)CSL_GIC_BASE_ADDR, intrNum);
}

int32_t Intc_IntAssignEdgeIntType(uint16_t intrNum)
{
	return CSL_gicAssignEdgeIntrType((uintptr_t)CSL_GIC_BASE_ADDR, intrNum);
}

int32_t Intc_IntSetAffinity(uint16_t intrNum)
{
	return CSL_gicSetIntrAffinity((uintptr_t)CSL_GIC_BASE_ADDR, intrNum);
}

void Intc_IntUnregister(uint16_t intrNum)
{
	/* Assign default ISR */
	gFnRAMVectors[intrNum] = (IntrFuncPtr)IntDefaultHandler;
	gArgArray[intrNum] = NULL;
}

void __Intc_Init(uint8_t cpuId)
{
	CSL_gicdRegs *gicdRegs = (CSL_gicdRegs *)(CSL_GIC_BASE_ADDR);
	CSL_gicrRegs_core_control *gicrRegs;
	CSL_gicrRegs_core_sgi_ppi *gicsRegs;
	int i, j;
	uint32_t intActiveReg;
	uint32_t routingMode = 0;
	uint8_t  aff0 = 0, aff1 = 0; /* By default, all interrupts route to CPU0. */

	/* Update the redistributor and sgi_ppi address */
	gicrRegs = (CSL_gicrRegs_core_control *)(uintptr_t)(CSL_GIC_BASE_ADDR + CSL_GICR_CORE_CONTROL_CTLR(cpuId));
	gicsRegs = (CSL_gicrRegs_core_sgi_ppi *)((uintptr_t)gicrRegs + (uintptr_t)0x10000U);

	/* Interrupt controller is already initialized */
	if (gCslA55InitControllerStatus == 1)
		return;

	/* Initialize the Interrupt controller */

	/* Disable IRQs */
	CSL_a55v8ArmGicv3HwiDisable();
	/* Enable interrupt controller system register access at EL1 */
	CSL_writeSystemReg(s3_0_c12_c12_5, 0x7); /* icc_sre_el1 */

	/*
	 * Disable forwarding of interrupts in GIC Distributer and CPU interface
	 * Controller.
	 */
	gicdRegs->CTLR = 0x0U;
	CSL_writeSystemReg(s3_0_c12_c12_7, 0x0); /* icc_igrpen1_el1 */

	/*
	 * Disable all interrupts at startup
	 */
	gicsRegs->ICENABLER0 = 0xFFFFFFFF;
	for (i = 0; i < 30; i++)
		gicdRegs->ICENABLER_SPI[i] = 0xFFFFFFFF;

	/*
	 * Enable forwarding of interrupts in GIC Distributor and CPU interface
	 * Controller.
	 */
	gicdRegs->CTLR = 0x2;
	CSL_writeSystemReg(s3_0_c12_c12_7, 0x1);   /* icc_igrpen1_el1 */

	/* Search for any previously active interrupts and acknowledge them */
	intActiveReg = gicsRegs->ICACTIVER0;
	if (intActiveReg) {
		for (j = 0; j < 32; j++) {
			if (intActiveReg & 0x1) {
				CSL_writeSystemReg(s3_0_c12_c12_1, j);   /* icc_eoir1_el1 */
			}
			intActiveReg = intActiveReg >> 1;
		}
	}

	for (i = 0; i < 30; i++) {
		intActiveReg = gicdRegs->ISACTIVER_SPI[i];
		if (intActiveReg) {
			for (j = 0; j < 32; j++) {
				if (intActiveReg & 0x1) {
					CSL_writeSystemReg(s3_0_c12_c12_1, (i * 32) + j); /* icc_eoir1_el1 */
				}
				intActiveReg = intActiveReg >> 1;
			}
		}
	}

	/*
	 * Clear any currently pending enabled interrupts
	 */
	gicsRegs->ICPENDR0  = 0xFFFFFFFF;
	for (i = 0; i < 30; i++)
		gicdRegs->ICPENDR_SPI[i] = 0xFFFFFFFF;

	/*
	 * Clear all interrupt active status registers
	 */
	gicsRegs->ICACTIVER0 = 0xFFFFFFFF;
	for (i = 0; i < 30; i++)
		gicdRegs->ICACTIVER_SPI[i] = 0xFFFFFFFF;

	/*
	 * Initialize the interrupt routing registers for all
	 * interrupts (32-1019).
	 *
	 * Interrupt number lower than 32 are not routed using these registers.
	 */
	for (i = 0; i < 960; i++) {
		gicdRegs->IROUTER[i].UPPER = routingMode;
		gicdRegs->IROUTER[i].LOWER =  (aff1 << 8 | aff0);
	}

	/*
	 * Initialize Binary Point Register
	 */
	CSL_writeSystemReg(s3_0_c12_c12_3, 3);   /* icc_bpr1_el1 */

	/*
	 * Set preconfigured interrupt Sense and Priority for each
	 * interrupt
	 */
	for (i = 0; i < 8; i++) {
		gicsRegs->IPRIORITYR[i] = 0x20202020;
		CSL_a55v8DsbSy();
	}
	for (i = 0; i < 240; i++) {
		gicdRegs->IPRIORITYR_SPI[i]= 0x20202020;
		CSL_a55v8DsbSy();
	}

	/*
	 * Initialize Interrupt Priority Mask Register
	 *
	 * Initialize PMR with the minimum possible interrupt priority.
	 */
	CSL_writeSystemReg(s3_0_c4_c6_0, 0xFF); /* icc_pmr_el1 */

	/*
	 * Set trigger sensitivity of interrupts
	 *
	 * On the Cortex-A15:
	 *  -   For SGI:
	 *          The ICFGR bits are read-only and a bit-pair always reads as
	 *          b10 because SGIs are edge-triggered.
	 *  -   For PPI and SPI :
	 *          The LSB of the bit-pair is read only and is always 0. MSB can
	 *          be altered to change trigger sensitivity.
	 *          b00    Interrupt is active-High level-sensitive
	 *          b10    Interrupt is rising edge-sensitive
	 */
	gicsRegs->ICFGR1 = 0;
	for (i = 0; i < (30 * 2); i++)
		gicdRegs->ICFGR_SPI[i] = 0;

	/* Record the initialization is done */
	gCslA55InitControllerStatus = 1;

	/* Register the default handlers */
	for (i = 0; i < NUM_INTERRUPTS; i++) {
		uintptr_t IntDefaultHandler_t = (uintptr_t)(&IntDefaultHandler);
		/* Assign default ISR */
		gFnRAMVectors[i] = (IntrFuncPtr) IntDefaultHandler_t;
		gArgArray[i]     = NULL;
	}

    return;
}

void __Intc_Redist_Init(uint8_t cpuId)
{
	CSL_gicrRegs_core_control *gicrRegs;
	CSL_gicrRegs_core_sgi_ppi *gicsRegs;
    int i, j;
	uint32_t intActiveReg;

    /* Update the redistributor and sgi_ppi address */
	gicrRegs = (CSL_gicrRegs_core_control *)(uintptr_t)(CSL_GIC_BASE_ADDR + CSL_GICR_CORE_CONTROL_CTLR(cpuId));
	gicsRegs = (CSL_gicrRegs_core_sgi_ppi *)((uintptr_t)gicrRegs + (uintptr_t)0x10000U);

	/* Interrupt controller is already initialized */
	if (gCslA55InitControllerStatus == 1)
		return;

	/* Initialize the Interrupt controller */

	/* Disable IRQs */
	CSL_a55v8ArmGicv3HwiDisable();
	/* Enable interrupt controller system register access at EL1 */
	CSL_writeSystemReg(s3_0_c12_c12_5, 0x7); /* icc_sre_el1 */

	/*
	 * Disable forwarding of interrupts in GIC Distributer and CPU interface
	 * Controller.
	 */
	CSL_writeSystemReg(s3_0_c12_c12_7, 0x0); /* icc_igrpen1_el1 */

	/*
	 * Disable all interrupts at startup
	 */
	gicsRegs->ICENABLER0 = 0xFFFFFFFF;

	/*
	 * Enable forwarding of interrupts in GIC Distributor and CPU interface
	 * Controller.
	 */
	CSL_writeSystemReg(s3_0_c12_c12_7, 0x1);   /* icc_igrpen1_el1 */

	/* Search for any previously active interrupts and acknowledge them */
	intActiveReg = gicsRegs->ICACTIVER0;
	if (intActiveReg) {
		for (j = 0; j < 32; j++) {
			if (intActiveReg & 0x1) {
				CSL_writeSystemReg(s3_0_c12_c12_1, j);   /* icc_eoir1_el1 */
			}
			intActiveReg = intActiveReg >> 1;
		}
	}

	/*
	 * Clear any currently pending enabled interrupts
	 */
	gicsRegs->ICPENDR0  = 0xFFFFFFFF;

	/*
	 * Clear all interrupt active status registers
	 */
	gicsRegs->ICACTIVER0 = 0xFFFFFFFF;

	/*
	 * Initialize the interrupt routing registers for all
	 * interrupts (32-1019).
	 *
	 * Interrupt number lower than 32 are not routed using these registers.
	 */


	/*
	 * Initialize Binary Point Register
	 */
	CSL_writeSystemReg(s3_0_c12_c12_3, 3);   /* icc_bpr1_el1 */

	/*
	 * Set preconfigured interrupt Sense and Priority for each
	 * interrupt
	 */
	for (i = 0; i < 8; i++) {
		gicsRegs->IPRIORITYR[i] = 0x20202020;
		CSL_a55v8DsbSy();
	}

	/*
	 * Initialize Interrupt Priority Mask Register
	 *
	 * Initialize PMR with the minimum possible interrupt priority.
	 */
	CSL_writeSystemReg(s3_0_c4_c6_0, 0xFF); /* icc_pmr_el1 */

	/*
	 * Set trigger sensitivity of interrupts
	 *
	 * On the Cortex-A15:
	 *  -   For SGI:
	 *          The ICFGR bits are read-only and a bit-pair always reads as
	 *          b10 because SGIs are edge-triggered.
	 *  -   For PPI and SPI :
	 *          The LSB of the bit-pair is read only and is always 0. MSB can
	 *          be altered to change trigger sensitivity.
	 *          b00    Interrupt is active-High level-sensitive
	 *          b10    Interrupt is rising edge-sensitive
	 */
	gicsRegs->ICFGR1 = 0;

	/* Record the initialization is done */
	gCslA55InitControllerStatus = 1;

	/* Register the default handlers */
	for (i = 0; i < NUM_INTERRUPTS; i++) {
		uintptr_t IntDefaultHandler_t = (uintptr_t)(&IntDefaultHandler);
		/* Assign default ISR */
		gFnRAMVectors[i] = (IntrFuncPtr)IntDefaultHandler_t;
		gArgArray[i]     = NULL;
    }

    return;
}

void Intc_Init(uint8_t cpuId)
{
	//__Intc_Init(cpuId);
	/* Note: In AMP mode, the GIC should not be reinitialized. */
	__Intc_Redist_Init(cpuId);
}

bool Intc_isInitialized(void)
{
	if (gCslA55InitControllerStatus != 0) {
		return true;
	} else {
		return false;
	}
}

int32_t Intc_IntPrioritySet(uint16_t intrNum, uint16_t priority)
{
	int32_t ret;
	ret = CSL_gicSetIntrPriority((uintptr_t)CSL_GIC_BASE_ADDR, intrNum, (uint8_t)priority);
	CSL_a55v8DsbSy();
	return ret;
}

uint32_t Intc_SystemEnable(uint16_t intrNum)
{
	uint32_t oldEnableState = 0U;
	uint64_t index, mask;
	uint64_t key;
	CSL_gicdRegs *gicdRegs = (CSL_gicdRegs *)(CSL_GIC_BASE_ADDR);

	key = CSL_a55v8ArmGicv3HwiDisable();
	index = intrNum / 32;
	mask = 1 << (intrNum & 0x1f);

	if (index > 0) {
		index--;
		oldEnableState = gicdRegs->ISENABLER_SPI[index] & mask;
		gicdRegs->ISENABLER_SPI[index] = mask;
	}
	CSL_a55v8ArmGicv3HwiRestore(key);

	CSL_a55v8ArmGicv3HwiEnable();

	return oldEnableState;
}

uint32_t Intc_SystemDisable(uint16_t intrNum)
{
	uint64_t key, index, mask;
	uint32_t oldEnableState = 0U;
	CSL_gicdRegs *gicdRegs = (CSL_gicdRegs *)(CSL_GIC_BASE_ADDR);

	key = CSL_a55v8ArmGicv3HwiDisable();
	index = intrNum / 32;
	mask = 1 << (intrNum & 0x1f);

	if (index > 0) {
		index--;
		oldEnableState = gicdRegs->ISENABLER_SPI[index] & mask;
		gicdRegs->ICENABLER_SPI[index] = mask;
	}

	CSL_a55v8ArmGicv3HwiRestore(key);

	return oldEnableState;
}
