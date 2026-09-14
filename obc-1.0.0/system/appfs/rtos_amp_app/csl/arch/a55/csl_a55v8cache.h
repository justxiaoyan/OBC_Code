/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_A55v8_CACHE_H_
#define _CSL_A55v8_CACHE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Instruction Cache Enable and Disable Operations */

void CSL_a55v8EnableInstEl1(void);
void CSL_a55v8EnableInstEl2(void);
void CSL_a55v8EnableInstEl3(void);
void CSL_a55v8DisableInstEl1(void);
void CSL_a55v8DisableInstEl2(void);
void CSL_a55v8DisableInstEl3(void);

/*  Data Cache Enable and Disable Operations */

void CSL_a55v8EnableDcacheEl1(void);
void CSL_a55v8EnableDcacheEl2(void);
void CSL_a55v8EnableDcacheEl3(void);
void CSL_a55v8DisableDcacheEl1(void);
void CSL_a55v8DisableDcacheEl2(void);
void CSL_a55v8DisableDcacheEl3(void);

/*  MMU Enable and Disable Operations */

void CSL_a55v8EnableMmuEL1(void);
void CSL_a55v8EnableMmuEl2(void);
void CSL_a55v8EnableMmuEl3(void);
void CSL_a55v8DisableMmuEL1(void);
void CSL_a55v8DisableMmuEl2(void);
void CSL_a55v8DisableMmuEl3(void);

/* Data Cache Clean and Invalidate Operations */

void CSL_a55v8CleanInvalidateDcacheSetWay(uint64_t setway);
void CSL_a55v8CleanInvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a55v8CleanDcacheSetWay(uint64_t setway);
void CSL_a55v8CleanDcacheMvaPoC(uint64_t mva);
void CSL_a55v8CleanDcacheMvaPoU(uint64_t mva);
void CSL_a55v8InvalidateDcacheSetWay(uint64_t setway);
void CSL_a55v8InvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a55v8_zeroDcaheMva(uint64_t mva);
void CSL_a55v8InvalidateIcacheAllPoU(void);
void CSL_a55v8InvalidateIcacheAllPoUIs(void);
void CSL_a55v8InvalidateIcacheLineMvaPoU(uint64_t mva);
void CSL_a55v8InvalidateDcache(uint64_t cache_num);

/* Instruction, Data Cache and MMU Enable and Disable Operations */

void CSL_a55v8InitIDCacheMmuEl1(void);
void CSL_a55v8InitIDCacheMmuEl2(void);
void CSL_a55v8InitIDCacheMmuEl3(void);

/* Instruction, Data Cache and MMU Enable and Disable Operations */

void CSL_a55v8EnableMairEl1(void);
void CSL_a55v8EnableMairEl2(void);
void CSL_a55v8EnableMairEl3(void);

/* Cache Levels and modes Operations */

uint32_t CSL_a55v8GetCacheLevelId(void);
void CSL_a55v8SetCacheLevelInD(uint32_t LevelInD);
uint32_t CSL_a55v8GetCacheLevelInD(void);
uint32_t CSL_a55v8GetCurrentCacheSize(void);

uint32_t CSL_a55v8FindNumLeadZeros(uint32_t param);

void CSL_a55v8WriteTCREL1(uint64_t tcr);
void CSL_a55v8WriteTCREL3(uint64_t tcr);

void CSL_a55v8SetMAIREL1(uint8_t idx, uint16_t attr);
void CSL_a55v8SetMAIREL3(uint8_t idx, uint16_t attr);

void CSL_a55v8InvTLB(void);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_A55v8_CACHE_H_ */
