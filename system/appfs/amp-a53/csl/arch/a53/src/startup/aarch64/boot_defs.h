/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_BOOT_DEFS_H_
#define _CSL_BOOT_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CSL_GIC_BASE_ADDR   0x01800000U	/* AM62x GICD */
/* Each Redistributor defines two 64KB frames in the physical address map. */
#define CSL_GICR_BASE_ADDR  0x01880000U	/* RD_base, core 0 */
#define CSL_GICS_BASE_ADDR  0x01890000U	/* SGI/PPI base, core 0 */

#define CSL_GICX_OFFSET     0x0020000 /* Offsets from CSL_GICR_BASE_ADDR */
#define CSL_STACK_SIZE		0x10000

/*
 * From version 3 of the GIC architecture, the ICC prefix indicates a GIC CPU interface System register.
 * See "ARM® Generic Interrupt Controller Architecture Specification" for more details.
 */

/* Interrupt Controller Interrupt Group 1 Enable Register (EL3) */
#ifndef icc_igrpen1_el3
#define icc_igrpen1_el3 s3_6_c12_c12_7
#endif

/* Interrupt Controller Interrupt Priority Mask Register */
#ifndef icc_pmr_el1
#define icc_pmr_el1     s3_0_c4_c6_0
#endif

/* Interrupt Controller System Register Enable Register (EL2) */
#ifndef icc_sre_el2
#define icc_sre_el2     s3_4_c12_c9_5
#endif

/* Interrupt Controller System Register Enable Register (EL3) */
#ifndef icc_sre_el3
#define icc_sre_el3     s3_6_c12_c12_5
#endif

#ifdef __cplusplus
}
#endif
#endif /* _CSL_BOOT_DEFS_H_ */
