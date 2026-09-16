/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_A53_H_
#define _CSL_A53_H_

#include <stdint.h>
#include "csl_a53v8misc.h"	/* for CSL_a53MmuDescAttrs_t */
#include "csl_a53v8cache.h"

#ifdef __cplusplus
extern "C" {
#endif

/*  Data Cache Enable and Disable Operations */

void CSL_a53DisableDcacheEl3(void);
void CSL_a53DisableDcacheEl2(void);
void CSL_a53DisableDcacheEl1(void);
void CSL_a53DisableDcacheAllLev(void);
void CSL_a53EnableDcacheEl3(void);
void CSL_a53EnableDcacheEl2(void);
void CSL_a53EnableDcacheEl1(void);
void CSL_a53EnableDcacheAllLev(void);

/* Instruction Cache Enable and Disable Operations */

void CSL_a53EnableIcacheEl3(void);
void CSL_a53EnableIcacheEl2(void);
void CSL_a53EnableIcacheEl1(void);
void CSL_a53EnableIcacheAllLev(void);
void CSL_a53DisableDataCoherency(void);

/* Data Cache Clean and Invalidate Operations */

void CSL_a53CleanInvalidateL1DcacheSetWay(uint64_t setway);
void CSL_a53CleanInvalidateL2DcacheSetWay(uint64_t setway);
void CSL_a53CleanInvalidateL1L2DcacheSetWay(uint64_t setway);
void CSL_a53CleanL1DcacheSetWay(uint64_t setway);
void CSL_a53CleanL2DcacheSetWay(uint64_t setway);
void CSL_a53CleanL1L2DcacheSetWay(uint64_t setway);
void CSL_a53InvalidateL1DcacheSetWay(uint64_t setway);
void CSL_a53InvalidateL2DcacheSetWay(uint64_t setway);
void CSL_a53InvalidateL1L2DcacheSetWay(uint64_t setway);
void CSL_a53CleanInvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a53InvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a53CleanDcacheMvaPoC(uint64_t mva);
void CSL_a53CleanDcacheMvaPoU(uint64_t mva);
void CSL_a53InvalidateL1DCache(void);
void CSL_a53InvalidateL2DCache(void);

void CSL_cacheEnable(void);
void CSL_cacheDisable(void);

void CSL_a53Isb(void);

uint64_t CSL_mmuInitLevel1Desc(uint64_t phyAddr, CSL_a53MmuDescAttrs_t *attrs);
uint64_t CSL_mmuInitLevel2Desc(uint64_t phyAddr, CSL_a53MmuDescAttrs_t *attrs);
void CSL_mmuSetMAIR(uint8_t idx, uint16_t attr);
void CSL_mmuInit(uintptr_t tableAddr);
void CSL_mmuSetTCR(uint64_t tcr);
void CSL_mmuStartup(void);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_A53_H_ */
