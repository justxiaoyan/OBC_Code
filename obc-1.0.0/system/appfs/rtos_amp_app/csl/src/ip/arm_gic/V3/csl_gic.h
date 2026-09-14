/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_GIC_H_
#define _CSL_GIC_H_

#include <stdint.h>
#include <stdbool.h>

#include "csl/cslr_gic600.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t CSL_gicAssignLevelIntrType(uintptr_t baseAddr, uint16_t intrNum);
int32_t CSL_gicAssignEdgeIntrType(uintptr_t baseAddr, uint16_t intrNum);

int32_t CSL_gicSetIntrPriority(uintptr_t baseAddr, uint16_t intrNum, uint8_t priority);

int32_t CSL_gicSetIntrAffinity(uintptr_t baseAddr, uint16_t intrNum);

int32_t CSL_gicSetPendingIntr(uintptr_t baseAddr, uint16_t intrNum);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_GIC_H_ */
