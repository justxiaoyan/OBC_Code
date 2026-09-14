/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

/*
 * GICv3 interrupt type:
 *   Software Generated Interrupts (SGIs) (ID  0-15)
 *   Private Peripheral Interrupts (PPIs) (ID 16-31)
 *   Shared Peripheral Interrupts (SPIs)  (ID 32-1019)
 *   Locality-specific Peripheral Interrupts (LPIs) (ID 8192+)
 */

#include <stdint.h>
#include <errno.h>

#include "../csl_gic.h"
#include "csl/arch/a55/csl_a55v8misc.h" /* for CSL_a55v8GetCpuId() */

static int32_t CSL_gicSetIntrType(uintptr_t baseAddr, uint16_t intrNum, uint8_t type)
{
	uint32_t t;
	uint32_t mask;
	uint32_t m;
	volatile uint32_t *ICFGR;

	if (intrNum < 16u) {
		/*
		 * Because SGIs are always treated as edge-triggered, the GICR_ICFGR0
		 * for SGIs behaves as Read-As-One, Writes Ignored (RAO/WI).
		 */
		return -EINVAL; /* SGIs can't be changed */
	}

	/* PPIs */
	if (intrNum < 32u) {
		CSL_gicrRegs *gicrRegs = (CSL_gicrRegs *)(baseAddr + CSL_GICR_CORE_CONTROL_CTLR(0u));
		uint64_t mpidr = CSL_a55v8GetCpuId();
		uint8_t core;
		/* Visinextek 919 platform: 0-7 Cores. See mpidr_el1 for more details. */
		core = (uint8_t)(mpidr >> 8) & (uint8_t)0xFFu; /* The cluster not considered */

		intrNum -= 16u; /* IntNum: [0- 15] */
		ICFGR = &gicrRegs->CORE[core].SGI_PPI.ICFGR1;
	} else {
		CSL_gicdRegs *gicdRegs = (CSL_gicdRegs *)(baseAddr);
		intrNum -= 32u;
		if (intrNum > 960) {
/* The Arm CoreLink GIC-600 supports up to 960 Shared Peripheral Interrupts (SPIs). */
			return -EINVAL;
		}
		ICFGR = &gicdRegs->ICFGR_SPI[(intrNum / 16u)];
	}

	t = CSL_REG32_FEXT(ICFGR, GICD_ICFGR_SPI_INT_TYPE);
	mask = 3u;
	/* Mask out other bits and extract the bits to be updated. */
	m = t & (mask << ((intrNum % 16u) * 2u));
	/* Zero-out the bits to be updated. */
	t = t ^ m;
	/* Write new value. */
	t = t | (type << ((intrNum % 16u) * 2u));
	CSL_REG32_FINS(ICFGR, GICD_ICFGR_SPI_INT_TYPE , t);

	return 0;
}

int32_t CSL_gicAssignLevelIntrType(uintptr_t baseAddr, uint16_t intrNum)
{
	return CSL_gicSetIntrType(baseAddr, intrNum, 0u);
}

int32_t CSL_gicAssignEdgeIntrType(uintptr_t baseAddr, uint16_t intrNum)
{
	return CSL_gicSetIntrType(baseAddr, intrNum, 3u);
}

int32_t CSL_gicSetIntrPriority(uintptr_t baseAddr, uint16_t intrNum, uint8_t priority)
{
	uint32_t t;
	uint32_t mask;
	uint32_t m;
	volatile uint32_t *IPRIORITYR;

	if (intrNum < 32) {
		CSL_gicrRegs *gicrRegs = (CSL_gicrRegs *)(baseAddr + CSL_GICR_CORE_CONTROL_CTLR(0u));
		uint64_t mpidr = CSL_a55v8GetCpuId();
		uint8_t core;
		/* Visinextek 919 platform: 0-7 Cores. See mpidr_el1 for more details. */
		core = (uint8_t)(mpidr >> 8) & (uint8_t)0xFFu; /* The cluster not considered */

		IPRIORITYR = &gicrRegs->CORE[core].SGI_PPI.IPRIORITYR[(intrNum / 4u)];
	} else {
		CSL_gicdRegs *gicdRegs = (CSL_gicdRegs *)(baseAddr);
		intrNum -= 32;
		if (intrNum > 960) {
/* The Arm CoreLink GIC-600 supports up to 960 Shared Peripheral Interrupts (SPIs). */
			return -EINVAL;
		}
		IPRIORITYR = &gicdRegs->IPRIORITYR_SPI[(intrNum / 4u)];
	}

	t = CSL_REG32_FEXT(IPRIORITYR, GICD_IPRIORITYR_SPI_PRIO);
	mask = 255u;
	/* Mask out other bits and extract the bits to be updated. */
	m = t & (mask << ((intrNum % 4u) * 8u));
	/* Zero out the bits to be updated. */
	t = t ^ m;
	/* Write new value. */
	t = t | (priority << ((intrNum % 4u) * 8u));
	CSL_REG32_FINS(IPRIORITYR, GICD_IPRIORITYR_SPI_PRIO, t);

	return 0;
}

int32_t CSL_gicSetIntrAffinity(uintptr_t baseAddr, uint16_t intrNum)
{
	CSL_gicdRegs *gicdRegs = (CSL_gicdRegs *)(baseAddr);
	uint64_t mpidr;

	/* PPIs are inherently local to a core (affinity is fixed) */
	if (intrNum < 32)
		return -EINVAL;

	mpidr = CSL_a55v8GetCpuId();

	intrNum -= 32;
	gicdRegs->IROUTER[intrNum].LOWER = (uint32_t)(mpidr & ~((uint64_t)(0xff) << 24));
	gicdRegs->IROUTER[intrNum].UPPER = (mpidr >> 32) & CSL_GICD_IROUTER_UPPER_A3_MASK;

	return 0;
}

int32_t CSL_gicSetPendingIntr(uintptr_t baseAddr, uint16_t intrNum)
{
	uint32_t t;
	uint32_t mask;
	uint32_t m;
	volatile uint32_t *ISPENDR;

	if (intrNum < 32) {
		CSL_gicrRegs *gicrRegs = (CSL_gicrRegs *)(baseAddr + CSL_GICR_CORE_CONTROL_CTLR(0u));
		uint64_t mpidr = CSL_a55v8GetCpuId();
		uint8_t core;
		/* Visinextek 919 platform: 0-7 Cores. See mpidr_el1 for more details. */
		core = (uint8_t)(mpidr >> 8) & (uint8_t)0xFFu; /* The cluster not considered */

		ISPENDR = &gicrRegs->CORE[core].SGI_PPI.ISPENDR0;
	} else {
		CSL_gicdRegs *gicdRegs = (CSL_gicdRegs *)(baseAddr);
		intrNum -= 32;
		if (intrNum > 960) {
/* The Arm CoreLink GIC-600 supports up to 960 Shared Peripheral Interrupts (SPIs). */
			return -EINVAL;
		}
		ISPENDR = &gicdRegs->ISPENDR_SPI[(intrNum / 32u)];
	}

	t = CSL_REG32_FEXT(ISPENDR, GICD_ISPENDR_SPI_PEND);
	mask = 1u;
	/* Mask out other bits and extract the bits to be updated. */
	m = t & (mask << (intrNum % 32u));
	/* Zero-out the bits to be updated. */
	t = t ^ m;
	/* Write new value. */
	t = t | (1u << (intrNum % 32u));
	CSL_REG32_FINS(ISPENDR, GICD_ISPENDR_SPI_PEND, t);

	return 0;
}
