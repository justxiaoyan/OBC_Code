/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/*
 * Stolen from ti/csl/arch/a53/src/startup/aarch64/csl_a55v8_startup.c.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "csl/arch/csl_arch.h"

extern void (*__init_array_start []) (void);
extern void (*__init_array_end []) (void);
extern unsigned int __bss_start__, __bss_end__;
extern unsigned int __data_load__, __data_start__, __data_end__;
extern int main();
uint64_t TTBR3_BASE_ADDR, TTBR2_BASE_ADDR, TTBR1_BASE_ADDR;

int start_boot(void)
{
	unsigned int *bs;
	unsigned int *be;
	unsigned int *dl;
	unsigned int *ds;
	unsigned int *de;
	uint64_t cpuId;
	uint8_t coreId;

	cpuId = CSL_a55v8GetCpuId();
	/* Visinextek 919 platform: 0-7 Cores. See mpidr_el1 for more details. */
	coreId = (uint8_t)(cpuId >> 8) & (uint8_t)0xFFU; /* The cluster not considered */

	/* initiailize .bss to zero */
	bs = &__bss_start__;
	be = &__bss_end__;
	while (bs < be) {
		*bs = 0;
		bs++;
	}

	/* relocate the .data section */
	dl = &__data_load__;
	ds = &__data_start__;
	de = &__data_end__;
	if (dl != ds) {
		while (ds < de) {
			*ds = *dl;
			dl++;
			ds++;
		}
	}

	/* Default: initialize mmu and cache */
	CSL_mmuStartup();

	/* Initialize the GIC V3 */
	Intc_Init(coreId);

	/* Calling the main */
	main();

	CSL_a55Exit();

	return 0;
}
