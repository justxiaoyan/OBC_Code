/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#ifndef _CSL_A55v8_MISC_H_
#define _CSL_A55v8_MISC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum CSL_a55MmuDescType {
	CSL_a55Mmu_DescType_INVALID0 = 0u,
	CSL_a55Mmu_DescType_BLOCK = 1u,
	CSL_a55Mmu_DescType_INVALID1 = 2u,
	CSL_a55Mmu_DescType_TABLE = 3u
};
typedef enum CSL_a55MmuDescType CSL_a55MmuDescType_t;

struct CSL_A55MmuDescAttrs {
	CSL_a55MmuDescType_t type;
	bool nsTable;
	uint8_t apTable;
	bool xnTable;
	bool pxnTable;
	bool noExecute;
	bool privNoExecute;
	bool contiguous;
	bool notGlobal;
	bool accessFlag;
	uint8_t shareable;
	uint8_t accPerm;
	bool nonSecure;
	uint8_t attrIndx;
	uint8_t reserved;
};
typedef struct CSL_A55MmuDescAttrs CSL_a55MmuDescAttrs_t;

enum mm_shareable_attr {
	MM_NO_SHAREABLE = 0,
	MM_NO_RESERVED  = 1,
	MM_OUTER_SHAREABLE = 2,
	MM_INNER_SHAREABLE = 3,	
};
typedef enum mm_shareable_attr mm_shareable_attr_t;

enum mm_access_permissions {
	EL1_RW 	   = 0,
	EL1_RDONLY = 2,
};
typedef enum mm_access_permissions mm_access_permissions_t;

enum mm_attribute {
	MM_DEVICE_NGNRNE = 0,
	MM_NORMAL_WBWA	 = 1,
	MM_NORMAL_NO_CACHED	= 2,	
};
typedef enum mm_attribute mm_attribute_t;

struct CSL_mmu_block_attribute {
	mm_shareable_attr_t	shareable;
	mm_access_permissions_t ap;
	mm_attribute_t attr;
	bool no_execute;
};
typedef struct CSL_mmu_block_attribute CSL_mmu_block_attribute_t;

void CSL_a55Exit(void);
void CSL_a55v8DsbSy();
void CSL_a55v8Isb(void);

uint64_t CSL_a55v8GetCpuId();

/* The 'tableAddr' is a physical address */
void CSL_a55v8WriteTtbr0El3(uintptr_t tableAddr);
void CSL_a55v8WriteTtbr0El2(uintptr_t tableAddr);
void CSL_a55v8WriteTtbr0El1(uintptr_t tableAddr);

void CSL_a55v8SwitchLevelEl3toEl2(uint64_t el2entry);
void CSL_a55v8SwitchLevelEl3toNsEl1(uint64_t el1entry);
void CSL_a55v8SwitchLevelEl3toSEl1(uint64_t el0entry);
void CSL_a55v8SwitchLevelEl3toNsEl0(uint64_t el0entry);
void CSL_a55v8SwitchLevelEl3toSEl0(uint64_t el1entry);
void CSL_a55v8SwitchLeveltoEl3(void);
void CSL_a55v8SwitchLevelEl0toEl3(void);
void CSL_a55v8SwitchLeveltoEl2(void);

/* Note: Only mask/unmask IRQ bit */
uint64_t CSL_a55v8ArmGicv3HwiDisable(void);
uint64_t CSL_a55v8ArmGicv3HwiEnable(void);
void CSL_a55v8ArmGicv3HwiRestore(uint64_t key);

#ifdef __cplusplus
}
#endif
#endif /* _CSL_A55v8_MISC_H_ */
