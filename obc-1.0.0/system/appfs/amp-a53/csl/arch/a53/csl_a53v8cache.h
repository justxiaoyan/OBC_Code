/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_A53v8_CACHE_H_
#define _CSL_A53v8_CACHE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Instruction Cache Enable and Disable Operations */

void CSL_a53v8EnableInstEl1(void);
void CSL_a53v8EnableInstEl2(void);
void CSL_a53v8EnableInstEl3(void);
void CSL_a53v8DisableInstEl1(void);
void CSL_a53v8DisableInstEl2(void);
void CSL_a53v8DisableInstEl3(void);

/*  Data Cache Enable and Disable Operations */

void CSL_a53v8EnableDcacheEl1(void);
void CSL_a53v8EnableDcacheEl2(void);
void CSL_a53v8EnableDcacheEl3(void);
void CSL_a53v8DisableDcacheEl1(void);
void CSL_a53v8DisableDcacheEl2(void);
void CSL_a53v8DisableDcacheEl3(void);

/*  MMU Enable and Disable Operations */

void CSL_a53v8EnableMmuEL1(void);
void CSL_a53v8EnableMmuEl2(void);
void CSL_a53v8EnableMmuEl3(void);
void CSL_a53v8DisableMmuEL1(void);
void CSL_a53v8DisableMmuEl2(void);
void CSL_a53v8DisableMmuEl3(void);

/* Data Cache Clean and Invalidate Operations */

void CSL_a53v8CleanInvalidateDcacheSetWay(uint64_t setway);
void CSL_a53v8CleanInvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a53v8CleanDcacheSetWay(uint64_t setway);
void CSL_a53v8CleanDcacheMvaPoC(uint64_t mva);
void CSL_a53v8CleanDcacheMvaPoU(uint64_t mva);
void CSL_a53v8InvalidateDcacheSetWay(uint64_t setway);
void CSL_a53v8InvalidateDcacheMvaPoC(uint64_t mva);
void CSL_a53v8_zeroDcaheMva(uint64_t mva);
void CSL_a53v8InvalidateIcacheAllPoU(void);
void CSL_a53v8InvalidateIcacheAllPoUIs(void);
void CSL_a53v8InvalidateIcacheLineMvaPoU(uint64_t mva);
void CSL_a53v8InvalidateDcache(uint64_t cache_num);

/* Instruction, Data Cache and MMU Enable and Disable Operations */

void CSL_a53v8InitIDCacheMmuEl1(void);
void CSL_a53v8InitIDCacheMmuEl2(void);
void CSL_a53v8InitIDCacheMmuEl3(void);

/* Instruction, Data Cache and MMU Enable and Disable Operations */

void CSL_a53v8EnableMairEl1(void);
void CSL_a53v8EnableMairEl2(void);
void CSL_a53v8EnableMairEl3(void);

/* Cache Levels and modes Operations */

uint32_t CSL_a53v8GetCacheLevelId(void);
void CSL_a53v8SetCacheLevelInD(uint32_t LevelInD);
uint32_t CSL_a53v8GetCacheLevelInD(void);
uint32_t CSL_a53v8GetCurrentCacheSize(void);

uint32_t CSL_a53v8FindNumLeadZeros(uint32_t param);

void CSL_a53v8WriteTCREL1(uint64_t tcr);
void CSL_a53v8WriteTCREL3(uint64_t tcr);

void CSL_a53v8SetMAIREL1(uint8_t idx, uint16_t attr);
void CSL_a53v8SetMAIREL3(uint8_t idx, uint16_t attr);

void CSL_a53v8InvTLB(void);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_A53v8_CACHE_H_ */
