/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSLR_GIC600_TOP_H_
#define _CSLR_GIC600_TOP_H_

#include "csl/cslr.h"

#define CSL_GIC600_BASE_ADDR	0x0d000000U

#define CSL_GIC600_GICR_CORE_CONTROL_OFFSET		(0x40000U)
#define CSL_GIC600_GICR_CORE_SGI_PPI_OFFSET		(0x50000U)

#define CSL_GIC_BASE_ADDR				CSL_GIC600_BASE_ADDR
#define CSL_GICR_CORE_CONTROL_OFFSET	CSL_GIC600_GICR_CORE_CONTROL_OFFSET
#define CSL_GICR_CORE_SGI_PPI_OFFSET	CSL_GIC600_GICR_CORE_SGI_PPI_OFFSET

#include "src/ip/arm_gic/V3/cslr_gic600.h"

#endif /* _CSLR_GIC600_TOP_H_ */
