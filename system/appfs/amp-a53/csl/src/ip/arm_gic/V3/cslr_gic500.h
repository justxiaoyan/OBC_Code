/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* AM62x Arm CoreLink GIC-500 register definitions (GICv3 architecture). */

#ifndef _CSLR_GIC500_H_
#define _CSLR_GIC500_H_

#include <stdint.h>

/*
 * Refer to
 *   1. "Arm CoreLink GIC-500 Generic Interrupt Controller Technical Reference Manual"
 *   2. "Arm Generic Interrupt Controller Architecture Specification, GICv3"
 */

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * GICv3 interrupt type:
 *   Software Generated Interrupts (SGIs) (ID  0-15)
 *   Private Peripheral Interrupts (PPIs) (ID 16-31)
 *   Shared Peripheral Interrupts (SPIs)  (ID 32-1019)
 *   Locality-specific Peripheral Interrupts (LPIs) (ID 8192+)
 */

/**************************************************************************
* Hardware Region  : GICD
**************************************************************************/

/*
 * Those registers are actually from GICv2, but the spec demands that they
 * are implemented as RES0 if ARE is 1.
 */
//#define GICD_ITARGETSR			0x0800
//#define GICD_SGIR					0x0F00
//#define GICD_CPENDSGIR			0x0F10
//#define GICD_SPENDSGIR			0x0F20 

/**************************************************************************
* Register Overlay Structure
**************************************************************************/

typedef struct {
	volatile uint32_t LOWER;
	volatile uint32_t UPPER;
} CSL_gicdRegs_irouter;

/************************************************************************
* Register Macros
**************************************************************************/

#define CSL_GICD_IROUTER_LOWER(IROUTER)		( 0x0U +  (0x6100U + ((IROUTER)*0x8U)) )
#define CSL_GICD_IROUTER_UPPER(IROUTER)		( 0x0U +  (0x6100U + ((IROUTER)*0x8U)) + 0x4U )

/************************************************************************
* Field Definition Macros
**************************************************************************/

/* LOWER */

#define CSL_GICD_IROUTER_LOWER_A0_MASK		(0xffU)
#define CSL_GICD_IROUTER_LOWER_A0_SHIFT		(0x0U)

#define CSL_GICD_IROUTER_LOWER_A1_MASK		(0xff00U)
#define CSL_GICD_IROUTER_LOWER_A1_SHIFT		(0x8U)

#define CSL_GICD_IROUTER_LOWER_IRM_MASK		(0x80000000U)
#define CSL_GICD_IROUTER_LOWER_IRM_SHIFT	(0x1fU)

#define CSL_GICD_IROUTER_LOWER_A2_MASK		(0xff0000U)
#define CSL_GICD_IROUTER_LOWER_A2_SHIFT		(0x10U)

/* UPPER */

#define CSL_GICD_IROUTER_UPPER_A3_MASK		(0xffU)
#define CSL_GICD_IROUTER_UPPER_A3_SHIFT		(0x0U)

/**************************************************************************
* Register Overlay Structure
**************************************************************************/


/*
 * Distributor registers, offsets from D_base
 */

typedef struct {
	volatile uint32_t  CTLR;					/* 0x0000 */
	volatile uint32_t  TYPER;					/* 0x0004 */
	volatile uint32_t  IIDR;					/* 0x0008 */
	volatile uint8_t   Rsvd0[20];				/* 0x000C */
	volatile uint8_t   Rsvd1[32];				/* 0x0020 */
	volatile uint32_t  SETSPI_NSR;				/* 0x0040 */
	volatile uint8_t   Rsvd2[4];				/* 0x0044 */
	volatile uint32_t  CLRSPI_NSR;				/* 0x0048 */
	volatile uint8_t   Rsvd3[4];				/* 0x004C */
	volatile uint32_t  SETSPI_SR;				/* 0x0050 */
	volatile uint8_t   Rsvd4[4];				/* 0x0054 */
	volatile uint32_t  CLRSPI_SR;				/* 0x0058 */
	volatile uint8_t   Rsvd5[36];				/* 0x005C */
	volatile uint32_t  IGROUPR_SGI_PPI;			/* 0x0080 */ /* 16 SGIs + 16 PPIs */
	volatile uint32_t  IGROUPR_SPI[30];			/* 0x0084 */ /* 960 SPIs */
	volatile uint8_t   Rsvd6[4];				/* 0x00FC */
	volatile uint32_t  ISENABLER_SGI_PPI;		/* 0x0100 */
	volatile uint32_t  ISENABLER_SPI[30];		/* 0x0104 */
	volatile uint8_t   Rsvd7[4];				/* 0x017C */
	volatile uint32_t  ICENABLER_SGI_PPI;		/* 0x0180 */
	volatile uint32_t  ICENABLER_SPI[30];		/* 0x0184 */
	volatile uint8_t   Rsvd8[4];				/* 0x01FC */
	volatile uint32_t  ISPENDR_SGI_PPI;			/* 0x0200 */
	volatile uint32_t  ISPENDR_SPI[30];			/* 0x0204 */
	volatile uint8_t   Rsvd9[4];				/* 0x027C */
	volatile uint32_t  ICPENDR_SGI_PPI;			/* 0x0280 */
	volatile uint32_t  ICPENDR_SPI[30];			/* 0x0284 */
	volatile uint8_t   Rsvd10[4];				/* 0x02FC */
	volatile uint32_t  ISACTIVER_SGI_PPI;		/* 0x0300 */
	volatile uint32_t  ISACTIVER_SPI[30];		/* 0x0304 */
	volatile uint8_t   Rsvd11[4];				/* 0x037C */
	volatile uint32_t  ICACTIVER_SGI_PPI;		/* 0x0380 */
	volatile uint32_t  ICACTIVER_SPI[30];		/* 0x0384 */
	volatile uint8_t   Rsvd12[4];				/* 0x03FC */
	volatile uint32_t  IPRIORITYR_SGI_PPI[8];	/* 0x0400 */
	volatile uint32_t  IPRIORITYR_SPI[240];		/* 0x0420 */
	volatile uint8_t   Rsvd13[1056];			/* 0x07E0 */
	volatile uint32_t  ICFGR_SGI_PPI[2];		/* 0x0C00 */
	volatile uint32_t  ICFGR_SPI[60];			/* 0x0C08 */
	volatile uint8_t   Rsvd14[8];				/* 0x0CF8 */
	volatile uint32_t  IGRPMODR_SGI_PPI;		/* 0x0D00 */
	volatile uint32_t  IGRPMODR_SPI[30];		/* 0x0D04 */
	volatile uint8_t   Rsvd15[132];				/* 0x0D7C */
	volatile uint32_t  NSACR[62];				/* 0x0E00 */
	volatile uint8_t   Rsvd16[21000];			/* 0x0EF8 */
	CSL_gicdRegs_irouter IROUTER[960];			/* 0x6100 */
	volatile uint8_t   Rsvd17[32976];			/* 0x7F00 */
	volatile uint32_t  PIDR4;					/* 0xFFD0 */
	volatile uint32_t  PIDR5;					/* 0xFFD4 */
	volatile uint32_t  PIDR6;					/* 0xFFD8 */
	volatile uint32_t  PIDR7;					/* 0xFFDC */
	volatile uint32_t  PIDR0;					/* 0xFFE0 */
	volatile uint32_t  PIDR1;					/* 0xFFE4 */
	volatile uint32_t  PIDR2;					/* 0xFFE8 */
	volatile uint32_t  PIDR3;					/* 0xFFEC */
	volatile uint32_t  CIDR0;					/* 0xFFF0 */
	volatile uint32_t  CIDR1;					/* 0xFFF4 */
	volatile uint32_t  CIDR2;					/* 0xFFF8 */
	volatile uint32_t  CIDR3;					/* 0xFFFC */
} CSL_gicdRegs;

/************************************************************************
* Field Definition Macros
**************************************************************************/

/* IPRIORITYR_SPI */

#define CSL_GICD_IPRIORITYR_SPI_PRIO_MASK	(0xffffffffU)
#define CSL_GICD_IPRIORITYR_SPI_PRIO_SHIFT	(0x0U)

#define CSL_GICD_ICFGR_SPI_INT_TYPE_MASK	(0xffffffffU)
#define CSL_GICD_ICFGR_SPI_INT_TYPE_SHIFT	(0x0U)

/* ISPENDR_SGI_PPI */

#define CSL_GICD_ISPENDR_SGI_PPI_PEND_MASK	(0xffffffffU)
#define CSL_GICD_ISPENDR_SGI_PPI_PEND_SHIFT	(0x0U)

/* ISPENDR_SPI */

#define CSL_GICD_ISPENDR_SPI_PEND_MASK		(0xffffffffU)
#define CSL_GICD_ISPENDR_SPI_PEND_SHIFT		(0x0U)

/**************************************************************************
* Hardware Region  : GICR
**************************************************************************/

/**************************************************************************
* Register Overlay Structure
**************************************************************************/

/*
 * Re-Distributor registers, offsets from RD_base
 */

typedef struct {
	volatile uint32_t  CTLR;					/* 0x0000 */
	volatile uint32_t  IIDR;					/* 0x0004 */
	volatile uint32_t  TYPER_LOWER;				/* 0x0008 */
	volatile uint32_t  TYPER_UPPER;				/* 0x000C */
	volatile uint8_t   Rsvd0[4];				/* 0x0010 */
	volatile uint32_t  WAKER;					/* 0x0014 */
	volatile uint8_t   Rsvd1[8];				/* 0x0018 */
	volatile uint8_t   Rsvd2[32];				/* 0x0020 */
	volatile uint32_t  SETLPIR_LOWER;			/* 0x0040 */
	volatile uint32_t  SETLPIR_UPPER;			/* 0x0044 */
	volatile uint32_t  CLRLPIR_LOWER;			/* 0x0048 */
	volatile uint32_t  CLRLPIR_UPPER;			/* 0x004C */
	volatile uint8_t   Rsvd3[32];				/* 0x0050 */
	volatile uint32_t  PROPBASER_LOWER;			/* 0x0070 */
	volatile uint32_t  PROPBASER_UPPER;			/* 0x0074 */
	volatile uint32_t  PENDBASER_LOWER;			/* 0x0078 */
	volatile uint32_t  PENDBASER_UPPER;			/* 0x007C */
	volatile uint8_t   Rsvd4[32];				/* 0x0080 */
	volatile uint32_t  INVLPIR_LOWER;			/* 0x00A0 */
	volatile uint32_t  INVLPIR_UPPER;			/* 0x00A4 */
	volatile uint8_t   Rsvd5[8];				/* 0x00A8 */
	volatile uint32_t  INVALLR_LOWER;			/* 0x00B0 */
	volatile uint32_t  INVALLR_UPPER;			/* 0x00B4 */
	volatile uint8_t   Rsvd6[8];				/* 0x00B8 */
	volatile uint32_t  SYNCR;					/* 0x00C0 */
	volatile uint8_t   Rsvd7[65292];			/* 0x00C4 */
	volatile uint32_t  PIDR4;					/* 0xFFD0 */
	volatile uint32_t  PIDR5;					/* 0xFFD4 */
	volatile uint32_t  PIDR6;					/* 0xFFD8 */
	volatile uint32_t  PIDR7;					/* 0xFFDC */
	volatile uint32_t  PIDR0;					/* 0xFFE0 */
	volatile uint32_t  PIDR1;					/* 0xFFE4 */	
	volatile uint32_t  PIDR2;					/* 0xFFE8 */
	volatile uint32_t  PIDR3;					/* 0xFFEC */
	volatile uint32_t  CIDR0;					/* 0xFFF0 */
	volatile uint32_t  CIDR1;					/* 0xFFF4 */
	volatile uint32_t  CIDR2;					/* 0xFFF8 */
	volatile uint32_t  CIDR3;					/* 0xFFFC */
} CSL_gicrRegs_core_control;

/************************************************************************
* Register Macros
**************************************************************************/

#define CSL_GICR_CORE_CONTROL_CTLR(CORE)		( CSL_GICR_CORE_CONTROL_OFFSET +  ((CORE)*0x20000U) )

/*
 * Re-Distributor registers, offsets from RD_base + 0x10000
 */

typedef struct {
	volatile uint8_t   Rsvd0[128];				/* 0x0000 */
	volatile uint32_t  IGROUPR_SGI_PPI;			/* 0x0080 */
	volatile uint8_t   Rsvd1[124];				/* 0x0084 */
	volatile uint32_t  ISENABLER0;				/* 0x0100 */
	volatile uint8_t   Rsvd2[124];				/* 0x0104 */
	volatile uint32_t  ICENABLER0;				/* 0x0180 */
	volatile uint8_t   Rsvd3[124];				/* 0x0184 */
	volatile uint32_t  ISPENDR0;				/* 0x0200 */
	volatile uint8_t   Rsvd4[124];				/* 0x0204 */
	volatile uint32_t  ICPENDR0;				/* 0x0280 */
	volatile uint8_t   Rsvd5[124];				/* 0x0284 */
	volatile uint32_t  ISACTIVER0;				/* 0x0300 */
	volatile uint8_t   Rsvd6[124];				/* 0x0304 */
	volatile uint32_t  ICACTIVER0;				/* 0x0380 */
	volatile uint8_t   Rsvd7[124];				/* 0x0384 */
	volatile uint32_t  IPRIORITYR[8];			/* 0x0400 */
	volatile uint8_t   Rsvd8[2016];				/* 0x0420 */
	volatile uint32_t  ICFGR0;					/* 0x0C00 */
	volatile uint32_t  ICFGR1;					/* 0x0C04 */
	volatile uint8_t   Rsvd9[248];				/* 0x0C08 */
	volatile uint32_t  IGRPMODR_SGI_PPI;		/* 0x0D00 */
	volatile uint8_t   Rsvd10[252];				/* 0x0D04 */
	volatile uint32_t  NSACR;					/* 0x0E00 */
	volatile uint8_t   Rsvd11[61900];			/* 0x0E04 */
	volatile uint32_t  PIDR4;					/* 0xFFD0 */
	volatile uint32_t  PIDR5;					/* 0xFFD4 */
	volatile uint32_t  PIDR6;					/* 0xFFD8 */
	volatile uint32_t  PIDR7;					/* 0xFFDC */
	volatile uint32_t  PIDR0;					/* 0xFFE0 */
	volatile uint32_t  PIDR1;					/* 0xFFE4 */
	volatile uint32_t  PIDR2;					/* 0xFFE8 */
	volatile uint32_t  PIDR3;					/* 0xFFEC */
	volatile uint32_t  CIDR0;					/* 0xFFF0 */
	volatile uint32_t  CIDR1;					/* 0xFFF4 */
	volatile uint32_t  CIDR2;					/* 0xFFF8 */
	volatile uint32_t  CIDR3;					/* 0xFFFC */
} CSL_gicrRegs_core_sgi_ppi;

/************************************************************************
* Field Definition Macros
**************************************************************************/

/* IPRIORITYR */

#define CSL_GICR_CORE_SGI_PPI_IPRIORITYR_PRIO_MASK		(0xffffffffU)
#define CSL_GICR_CORE_SGI_PPI_IPRIORITYR_PRIO_SHIFT		(0x0U)

/* ISPENDR0 */

#define CSL_GICR_CORE_SGI_PPI_ISPENDR0_PEND_MASK		(0xffffffffU)
#define CSL_GICR_CORE_SGI_PPI_ISPENDR0_PEND_SHIFT		(0x0U)

/**************************************************************************
* Register Overlay Structure
**************************************************************************/

/* Each Redistributor defines two 64KB frames in the physical address map */
typedef struct {
	CSL_gicrRegs_core_control CONTROL;	/* Offset 0x0 */
	CSL_gicrRegs_core_sgi_ppi SGI_PPI;	/* Offset 0x10000 */
} CSL_gicrRegs_core;

typedef struct {
	CSL_gicrRegs_core CORE[4];	/* AM62x Cortex-A53 cores */
} CSL_gicrRegs;

#ifdef __cplusplus
}
#endif
#endif /* _CSLR_GIC500_H_ */
