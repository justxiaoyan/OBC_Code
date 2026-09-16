/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSLR_GIC500_TOP_H_
#define _CSLR_GIC500_TOP_H_

#include "csl/cslr.h"

#define CSL_GIC500_BASE_ADDR	0x01800000U

#define CSL_GIC500_GICR_CORE_CONTROL_OFFSET		(0x80000U)
#define CSL_GIC500_GICR_CORE_SGI_PPI_OFFSET		(0x90000U)

#define CSL_GIC_BASE_ADDR				CSL_GIC500_BASE_ADDR
#define CSL_GICR_CORE_CONTROL_OFFSET	CSL_GIC500_GICR_CORE_CONTROL_OFFSET
#define CSL_GICR_CORE_SGI_PPI_OFFSET	CSL_GIC500_GICR_CORE_SGI_PPI_OFFSET

#include "src/ip/arm_gic/V3/cslr_gic500.h"

#endif /* _CSLR_GIC500_TOP_H_ */
