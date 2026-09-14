/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/*
 * Stolen from ti/csl/arch/a53/src/csl_a53.c. One difference is that
 * this only supports ARMv8 A64 instruction set.
 */

#include "csl/arch/a55/csl_a55.h"			//../csl_a55.h
#include "csl/arch/a55/csl_a55v8cache.h"	//../csl_a55v8cache.h
#include "csl/arch/a55/csl_a55v8misc.h"		//../csl_a55v8misc.h

#define u8  uint8_t
#define u32 uint32_t
#define u64 uint64_t

#define IRQ_FIQ_MASK 0xC0U	/* Mask IRQ and FIQ interrupts in cpsr */

/* pseudo assembler instructions */
#define mfcpsr()	({u32 rval = 0U; \
		__asm volatile("mrs %0,  DAIF" : "=r" (rval));\
		rval;\
	})

#define mtcpsr(v) __asm__ __volatile__ ("msr DAIF, %0" : : "r" (v))

#define cpsiei()	//__asm__ __volatile__("cpsie	i\n")
#define cpsidi()	//__asm__ __volatile__("cpsid	i\n")

#define cpsief()	//__asm__ __volatile__("cpsie	f\n")
#define cpsidf()	//__asm__ __volatile__("cpsid	f\n")

#define mtgpr(rn, v)	/*__asm__ __volatile__(\
			  "mov r" stringify(rn) ", %0 \n"\
			  : : "r" (v)\
)*/

#define mfgpr(rn)	/*({u32 rval; \
			  __asm__ __volatile__(\
			    "mov %0,r" stringify(rn) "\n"\
			    : "=r" (rval)\
			  );\
			  rval;\
})*/

/* memory synchronization operations */

/* Instruction Synchronization Barrier */
#define isb() __asm__ __volatile__ ("isb sy")

/* Data Synchronization Barrier */
#define dsb() __asm__ __volatile__("dsb sy")

/* Data Memory Barrier */
#define dmb() __asm__ __volatile__("dmb sy")

/* Memory Operations */
#define ldr(adr)	({u64 rval; \
		__asm__ __volatile__(\
				     "ldr	%0,[%1]"\
				     : "=r" (rval) : "r" (adr)\
				    );\
		rval;\
	})

#define mfelrel3() ({u64 rval = 0U; \
				__asm volatile("mrs %0,  ELR_EL3" : "=r" (rval));\
				rval;\
			})
#define mtelrel3(v) __asm__ __volatile__ ("msr ELR_EL3, %0" : : "r" (v))

/* Count leading zeroes (clz) */
#define clz(arg)	({u8 rval; \
		__asm__ __volatile__(\
				     "clz	%0,%1"\
				     : "=r" (rval) : "r" (arg)\
				    );\
		rval;\
	})

#define mtcpdc(reg,val)	__asm__ __volatile__("dc " #reg ",%0"  : : "r" (val))
#define mtcpic(reg,val)	__asm__ __volatile__("ic " #reg ",%0"  : : "r" (val))

#define mtcpicall(reg)	__asm__ __volatile__("ic " #reg)
#define mtcptlbi(reg)	__asm__ __volatile__("tlbi " #reg)
#define mtcpat(reg,val)	__asm__ __volatile__("at " #reg ",%0"  : : "r" (val))
/* CP15 operations */
#define mfcp(reg)	({u64 rval = 0U;\
		__asm__ __volatile__("mrs	%0, " #reg : "=r" (rval));\
		rval;\
	})

#define mtcp(reg,val)	__asm__ __volatile__("msr " #reg ",%0"  : : "r" (val))

/*  Data Cache Enable and Disable Operations */

void CSL_a55DisableDcacheEl3(void)
{
	CSL_a55v8DisableDcacheEl3();
	CSL_a55Isb();
}

void CSL_a55DisableDcacheEl2(void)
{
	CSL_a55v8DisableDcacheEl2();
	CSL_a55Isb();
}

void CSL_a55DisableDcacheEl1(void)
{
	CSL_a55v8DisableDcacheEl1();
 	CSL_a55Isb();
}

void CSL_a55DisableDcacheAllLev(void)
{
	CSL_a55v8DisableDcacheEl3();
	CSL_a55v8DisableDcacheEl2();
	CSL_a55v8DisableDcacheEl1();
 	CSL_a55Isb();
}

void CSL_a55EnableDcacheEl3(void)
{
	CSL_a55v8EnableDcacheEl3();
	CSL_a55Isb();
}

void CSL_a55EnableDcacheEl2(void)
{
	CSL_a55v8EnableDcacheEl2();
	CSL_a55Isb();
}

void CSL_a55EnableDcacheEl1(void)
{
	CSL_a55v8EnableDcacheEl1();
	CSL_a55Isb();
}

void CSL_a55EnableDcacheAllLev(void)
{
	CSL_a55v8EnableDcacheEl3();
	CSL_a55v8EnableDcacheEl2();
	CSL_a55v8EnableDcacheEl1();
    CSL_a55Isb();
}

/* Instruction Cache Enable and Disable Operations */

void CSL_a55EnableIcacheEl3(void)
{
	CSL_a55v8EnableInstEl3();
	CSL_a55Isb();
}

void CSL_a55EnableIcacheEl2(void)
{
	CSL_a55v8EnableInstEl2();
	CSL_a55Isb();
}

void CSL_a55EnableIcacheEl1(void)
{
	CSL_a55v8EnableInstEl1();
	CSL_a55Isb();
}

void CSL_a55EnableIcacheAllLev(void)
{
	CSL_a55v8EnableInstEl3();
	CSL_a55v8EnableInstEl2();
	CSL_a55v8EnableInstEl1();
	CSL_a55Isb();
}

/* Data Cache Clean and Invalidate Operations */


void CSL_a55CleanInvalidateL1DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;
	LevelInD = 0x00000000U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanInvalidateDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55CleanInvalidateL2DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;
	LevelInD = 0x00000002U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanInvalidateDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55CleanInvalidateL1L2DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;

	/* Set current cache to L1 */
	LevelInD = 0x00000000U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanInvalidateDcacheSetWay(setway);
    CSL_a55Isb();
    /* Set current cache to L2 */
	LevelInD = 0x00000002U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanInvalidateDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55CleanL1DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;
	LevelInD = 0x00000000U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55CleanL2DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;
	LevelInD = 0x00000002U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55CleanL1L2DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;

	/* Set current cache to L1 */
	LevelInD = 0x00000000U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanInvalidateDcacheSetWay(setway);
	CSL_a55Isb();
	/* Set current cache to L2 */
	LevelInD = 0x00000002U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8CleanInvalidateDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55InvalidateL1DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;
	LevelInD = 0x00000000U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8InvalidateDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55InvalidateL2DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;
	LevelInD = 0x00000002U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8InvalidateDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55InvalidateL1L2DcacheSetWay(uint64_t setway)
{
	uint32_t    LevelInD;

	/* Set current cache to L1 */
	LevelInD = 0x00000000U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8InvalidateDcacheSetWay(setway);
	CSL_a55Isb();
	/* Set current cache to L2 */
	LevelInD = 0x00000002U;        /* 3:1 = Level 0 = InD */
	CSL_a55v8SetCacheLevelInD(LevelInD);
	CSL_a55Isb();
	CSL_a55v8InvalidateDcacheSetWay(setway);
	CSL_a55Isb();
}

void CSL_a55CleanInvalidateDcacheMvaPoC(uint64_t mva)
{
	CSL_a55v8CleanInvalidateDcacheMvaPoC(mva);
	CSL_a55Isb();
}

void CSL_a55InvalidateDcacheMvaPoC(uint64_t mva)
{
	CSL_a55v8InvalidateDcacheMvaPoC(mva);
	CSL_a55Isb();
}

void CSL_a55CleanDcacheMvaPoC(uint64_t mva)
{
	CSL_a55v8CleanDcacheMvaPoC(mva);
	CSL_a55Isb();
}

void CSL_a55CleanDcacheMvaPoU(uint64_t mva)
{
	CSL_a55v8CleanDcacheMvaPoU(mva);
	CSL_a55Isb();
}

void CSL_a55InvalidateL2DCache(void)
{
	CSL_a55v8InvalidateDcache(0x2);
	CSL_a55Isb();
}

void CSL_a55InvalidateL1DCache(void)
{
	CSL_a55v8InvalidateDcache(0x1);
	CSL_a55Isb();
}

void CSL_a55Isb(void)
{
	CSL_a55v8Isb();
}

/*
 * This function enables the instructions and data cache
 * when application enables the cache.
 */
void CSL_cacheDisable(void)
{
	CSL_a55v8DisableDcacheEl1();
	CSL_a55v8DisableInstEl1();
}

/*
 * This function enables the instructions and data cache
 * when application enables the cache.
 */
void CSL_cacheEnable(void)
{
	CSL_a55v8EnableDcacheEl1();
	CSL_a55v8EnableInstEl1();
}

void CSL_DCacheFlush(void)
{
	register uint32_t CsidReg, C7Reg;
	uint32_t LineSize, NumWays;
	uint32_t Way, WayIndex,WayAdjust, Set, SetIndex, NumSet, CacheLevel;
	uint32_t currmask;

	currmask = mfcpsr();
	mtcpsr(currmask | IRQ_FIQ_MASK);

	/* Number of level of cache*/

	CacheLevel = 0U;
	/* Select cache level 0 and D cache in CSSR */
	mtcp(CSSELR_EL1, CacheLevel);
	isb();

	CsidReg = mfcp(CCSIDR_EL1);

	/* Get the cacheline size, way size, index size from csidr */
	LineSize = (CsidReg & 0x00000007U) + 0x00000004U;

	/* Number of Ways */
	NumWays = (CsidReg & 0x00001FFFU) >> 3U;
	NumWays += 0x00000001U;

	/*Number of Set*/
	NumSet = (CsidReg >> 13U) & 0x00007FFFU;
	NumSet += 0x00000001U;

	WayAdjust = clz(NumWays) - (u32)0x0000001FU;

	Way = 0U;
	Set = 0U;

	/* Flush all the cachelines */
	for (WayIndex = 0U; WayIndex < NumWays; WayIndex++) {
		for (SetIndex = 0U; SetIndex < NumSet; SetIndex++) {
			C7Reg = Way | Set | CacheLevel;
			mtcpdc(CISW, C7Reg);
			Set += (0x00000001U << LineSize);
		}
		Set = 0U;
		Way += (0x00000001U << WayAdjust);
	}

	/* Wait for Flush to complete */
	dsb();

	/* Select cache level 1 and D cache in CSSR */
	CacheLevel += (0x00000001U << 1U);
	mtcp(CSSELR_EL1, CacheLevel);
	isb();

	CsidReg = mfcp(CCSIDR_EL1);

	/* Get the cacheline size, way size, index size from csidr */
	LineSize = (CsidReg & 0x00000007U) + 0x00000004U;

	/* Number of Ways */
	NumWays = (CsidReg & 0x00001FFFU) >> 3U;
	NumWays += 0x00000001U;

	/* Number of Sets */
	NumSet = (CsidReg >> 13U) & 0x00007FFFU;
	NumSet += 0x00000001U;

	WayAdjust = clz(NumWays) - (u32)0x0000001FU;

	Way = 0U;
	Set = 0U;

	/* Flush all the cachelines */
	for (WayIndex = 0U; WayIndex < NumWays; WayIndex++) {
		for (SetIndex = 0U; SetIndex < NumSet; SetIndex++) {
			C7Reg = Way | Set | CacheLevel;
			mtcpdc(CISW, C7Reg);
			Set += (0x00000001U << LineSize);
		}
		Set = 0U;
		Way += (0x00000001U << WayAdjust);
	}
	/* Wait for Flush to complete */
	dsb();

	mtcpsr(currmask);
}

/* visinextek 919: 1-64GB */
#define DDR_BASE_ADDR	0x40000000ULL
#define DDR_BASE_SIZE	0x200000000ULL	/* 8GB, this size is sufficient for all products. */

#define BLOCK_SIZE_2MB	0x200000U
#define BLOCK_SIZE_1GB	0x40000000U

#define CSL_a55Mmu_NUM_LEVEL1_ENTRIES	(512U)	/* 1G * 512 == 512GB */
#define CSL_a55Mmu_NUM_LEVEL2_ENTRIES	(4096U)	/* 2MB * 4096 == 8GB */

__attribute__((aligned(4096))) uint64_t gCSLa55Mmulevel1Table[CSL_a55Mmu_NUM_LEVEL1_ENTRIES];
__attribute__((aligned(4096))) uint64_t gCSLa55Mmulevel2Table[CSL_a55Mmu_NUM_LEVEL2_ENTRIES];
//__attribute__((aligned(4096))) uint64_t gCSLa55Mmulevel2Table[CSL_a55Mmu_NUM_LEVEL3_ENTRIES];

static inline int CSL_l2tableIndex(uint64_t addr)
{
	return ((addr - DDR_BASE_ADDR) / BLOCK_SIZE_2MB);
}

void CSL_setL2TlbAttributes(uintptr_t addr, CSL_mmu_block_attribute_t* attr)
{
	uint64_t desc;
	CSL_a55MmuDescAttrs_t attrs;

	attrs.type = CSL_a55Mmu_DescType_BLOCK;
	attrs.nsTable        = false;
	attrs.apTable        = 0u;
	attrs.xnTable        = false;
	attrs.pxnTable       = false;
	attrs.noExecute      = true;
	attrs.privNoExecute  = attr->no_execute;
	attrs.contiguous     = false;
	attrs.notGlobal      = false;
	attrs.accessFlag     = true;
	attrs.shareable      = attr->shareable;   
	attrs.accPerm        = attr->ap;  			
	attrs.nonSecure      = true;
	attrs.attrIndx       = attr->attr;
	attrs.reserved       = 0u;

	desc = CSL_mmuInitLevel2Desc(addr, &attrs);
	gCSLa55Mmulevel2Table[CSL_l2tableIndex(addr)] = desc;	

	CSL_DCacheFlush();

	isb();
	dsb();
}

void CSL_mmuStartup(void)
{
	uint64_t addr, desc;
	CSL_a55MmuDescAttrs_t attrs;

	/* Device memory attributes for 0x0 - 0x3FFFFFFF address space */
	attrs.type = CSL_a55Mmu_DescType_BLOCK;
	attrs.nsTable        = false;
	attrs.apTable        = 0u;
	attrs.xnTable        = false;
	attrs.pxnTable       = false;
	attrs.noExecute      = true;
	attrs.privNoExecute  = false;
	attrs.contiguous     = false;
	attrs.notGlobal      = false;
	attrs.accessFlag     = true;
	attrs.shareable      = MM_OUTER_SHAREABLE;   /* Outer shareable */
	attrs.accPerm        = 0x10;  			   /* RW at EL1 only */
	attrs.nonSecure      = true;
	attrs.attrIndx       = 0;
	attrs.reserved       = 0u;
	desc = CSL_mmuInitLevel1Desc(0x0, &attrs);
	gCSLa55Mmulevel1Table[0] = desc;

	attrs.type = CSL_a55Mmu_DescType_TABLE;
	attrs.nsTable = false;
	attrs.apTable = 0;
	attrs.xnTable = false;
	attrs.pxnTable = false;
	for (addr = DDR_BASE_ADDR; addr < DDR_BASE_ADDR + DDR_BASE_SIZE; addr += BLOCK_SIZE_1GB) {  	  
		desc = CSL_mmuInitLevel1Desc((uint64_t)&gCSLa55Mmulevel2Table[CSL_l2tableIndex(addr)], &attrs);
		gCSLa55Mmulevel1Table[addr / BLOCK_SIZE_1GB] = desc;  	
	}

	attrs.type = CSL_a55Mmu_DescType_BLOCK;
	attrs.pxnTable       = false;
	attrs.noExecute      = true;
	attrs.privNoExecute  = false;
	attrs.contiguous     = false;
	attrs.notGlobal      = false;
	attrs.accessFlag     = true;
	attrs.shareable      = 0x2;  /* Outer shareable */
	attrs.accPerm        = 0x10; /* RW at EL1 only */
	attrs.nonSecure      = true;
	attrs.attrIndx       = 1;
	attrs.reserved       = 0u;
	for (addr = DDR_BASE_ADDR; addr < DDR_BASE_ADDR + DDR_BASE_SIZE; addr += BLOCK_SIZE_2MB) {  	  
		desc = CSL_mmuInitLevel2Desc(addr, &attrs);
		gCSLa55Mmulevel2Table[CSL_l2tableIndex(addr)] = desc;  	
	}

	/**********************************************
	* Set up memory attributes
	* This equates to:
	* 0 = b01000100 = Device-nGnRnE
	* 1 = b11111111 = Normal, Inner/Outer WB/WA/RA
	* 2 = b01000100 = Normal, Inner/Outer Non-Cacheable
	**********************************************/
	CSL_mmuSetMAIR(0, 0x00);
	CSL_mmuSetMAIR(1, 0xFF); 
	CSL_mmuSetMAIR(2, 0x44);

	CSL_mmuInit((uintptr_t)&gCSLa55Mmulevel1Table[0]);

	/*
	 * Physical Address Size PS =  101 -> 48bits 256TB
	 * Granual Size TG0 = 00 -> 4KB
	 * size offset of the memory region T0SZ = (region size 2^(64-25) = 2^39) 
	 */
	CSL_a55v8WriteTCREL1(0x500000019);

	CSL_a55v8InitIDCacheMmuEl1();
}

uint64_t CSL_mmuInitLevel1Desc(uint64_t phyAddr, CSL_a55MmuDescAttrs_t *attrs)
{
	uint64_t desc;

	if (attrs->type == CSL_a55Mmu_DescType_BLOCK) {
		desc = ((uint64_t)attrs->type & 0x3) |
			   ((uint64_t)(attrs->attrIndx & 0x7) << 2) |
			   ((uint64_t)(attrs->nonSecure & 0x1) << 5) |
			   ((uint64_t)(attrs->accPerm & 0x3) << 6) |
			   ((uint64_t)(attrs->shareable & 0x3) << 8) |
			   ((uint64_t)(attrs->accessFlag & 0x1) << 10) |
			   ((uint64_t)(attrs->notGlobal & 0x1) << 11) |
			   ((uint64_t)phyAddr & 0xFFFFFFFFC0000000) |
			   ((uint64_t)(attrs->contiguous & 0x1) << 52) |
			   ((uint64_t)(attrs->privNoExecute & 0x1) << 53) |
			   ((uint64_t)(attrs->noExecute & 0x1) << 54) |
			   ((uint64_t)(attrs->reserved & 0xF) << 55);
	} else {
		desc = ((uint64_t)attrs->type & 0x3) |
			   ((uint64_t)phyAddr & 0xFFFFFFFFFFFFF000) |
			   ((uint64_t)(attrs->pxnTable & 0x1) << 59) |
			   ((uint64_t)(attrs->xnTable & 0x1) << 60) |
			   ((uint64_t)(attrs->apTable & 0x3) << 61) |
			   ((uint64_t)(attrs->nsTable & 0x1) << 63);
	}

    return desc;
}

uint64_t CSL_mmuInitLevel2Desc(uint64_t phyAddr, CSL_a55MmuDescAttrs_t *attrs)
{
    uint64_t desc;

    if (attrs->type == CSL_a55Mmu_DescType_BLOCK) {
        desc = ((uint64_t)attrs->type & 0x3) |
               ((uint64_t)(attrs->attrIndx & 0x7) << 2) |
               ((uint64_t)(attrs->nonSecure & 0x1) << 5) |
               ((uint64_t)(attrs->accPerm & 0x3) << 6) |
               ((uint64_t)(attrs->shareable & 0x3) << 8) |
               ((uint64_t)(attrs->accessFlag & 0x1) << 10) |
               ((uint64_t)(attrs->notGlobal & 0x1) << 11) |
               ((uint64_t)phyAddr & 0xFFFFFFFFFFE00000) |
               ((uint64_t)(attrs->contiguous & 0x1) << 52) |
               ((uint64_t)(attrs->privNoExecute & 0x1) << 53) |
               ((uint64_t)(attrs->noExecute & 0x1) << 54) |
               ((uint64_t)(attrs->reserved & 0xF) << 55);
	} else {
        desc = ((uint64_t)attrs->type & 0x3) |
               ((uint64_t)phyAddr & 0xFFFFFFFFFFFFF000) |
               ((uint64_t)(attrs->pxnTable & 0x1) << 59) |
               ((uint64_t)(attrs->xnTable & 0x1) << 60) |
               ((uint64_t)(attrs->apTable & 0x3) << 61) |
               ((uint64_t)(attrs->nsTable & 0x1) << 63);
    }

	return desc;
}

void CSL_mmuSetMAIR(uint8_t idx, uint16_t attr)
{
	CSL_a55v8SetMAIREL1(idx, attr);
	CSL_a55v8InvTLB();
}

void CSL_mmuInit(uintptr_t tableAddr)
{
	CSL_a55v8WriteTtbr0El1(tableAddr);
}

void CSL_mmuSetTCR(uint64_t tcr)
{
	CSL_a55v8WriteTCREL1(tcr);
}
