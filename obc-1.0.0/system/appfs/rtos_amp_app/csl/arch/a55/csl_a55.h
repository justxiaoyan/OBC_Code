/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_A55_H_
#define _CSL_A55_H_

#include <stdint.h>
#include "csl_a55v8misc.h"	/* for CSL_a55MmuDescAttrs_t */
#include "csl_a55v8cache.h"

#ifdef __cplusplus
extern "C" {
#endif

/*  Data Cache Enable and Disable Operations */

void CSL_a55DisableDcacheEl3(void);
void CSL_a55DisableDcacheEl2(void);
void CSL_a55DisableDcacheEl1(void);
void CSL_a55DisableDcacheAllLev(void);
void CSL_a55EnableDcacheEl3(void);
void CSL_a55EnableDcacheEl2(void);
void CSL_a55EnableDcacheEl1(void);
void CSL_a55EnableDcacheAllLev(void);

/* Instruction Cache Enable and Disable Operations */

void CSL_a55EnableIcacheEl3(void);
void CSL_a55EnableIcacheEl2(void);
void CSL_a55EnableIcacheEl1(void);
void CSL_a55EnableIcacheAllLev(void);
void CSL_a55DisableDataCoherency(void);

/* Data Cache Clean and Invalidate Operations */

void CSL_a55CleanInvalidateL1DcacheSetWay(uint64_t setway);
void CSL_a55CleanInvalidateL2DcacheSetWay(uint64_t setway);
void CSL_a55CleanInvalidateL1L2DcacheSetWay(uint64_t setway);
void CSL_a55CleanL1DcacheSetWay(uint64_t setway);
void CSL_a55CleanL2DcacheSetWay(uint64_t setway);
void CSL_a55CleanL1L2DcacheSetWay(uint64_t setway);
void CSL_a55InvalidateL1DcacheSetWay(uint64_t setway);
void CSL_a55InvalidateL2DcacheSetWay(uint64_t setway);
void CSL_a55InvalidateL1L2DcacheSetWay(uint64_t setway);
void CSL_a55CleanInvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a55InvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a55CleanDcacheMvaPoC(uint64_t mva);
void CSL_a55CleanDcacheMvaPoU(uint64_t mva);
void CSL_a55InvalidateL1DCache(void);
void CSL_a55InvalidateL2DCache(void);

void CSL_cacheEnable(void);
void CSL_cacheDisable(void);

void CSL_a55Isb(void);

uint64_t CSL_mmuInitLevel1Desc(uint64_t phyAddr, CSL_a55MmuDescAttrs_t *attrs);
uint64_t CSL_mmuInitLevel2Desc(uint64_t phyAddr, CSL_a55MmuDescAttrs_t *attrs);
void CSL_mmuSetMAIR(uint8_t idx, uint16_t attr);
void CSL_mmuInit(uintptr_t tableAddr);
void CSL_mmuSetTCR(uint64_t tcr);
void CSL_mmuStartup(void);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_A55_H_ */
