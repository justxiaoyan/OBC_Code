// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017-2019, 2021-2022 NXP
 *
 * Peng Fan <peng.fan@nxp.com>
 */

#include <common.h>
#include <cpu_func.h>
#include <event.h>
#include <init.h>
#include <log.h>
#include <asm/arch/imx-regs.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/hab.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/optee.h>
#include <asm/mach-imx/syscounter.h>
#include <asm/ptrace.h>
#include <asm/armv8/mmu.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <efi_loader.h>
#include <env.h>
#include <env_internal.h>
#include <errno.h>
#include <fdt_support.h>
#include <fsl_wdog.h>
#include <fuse.h>
#include <imx_sip.h>
#include <linux/arm-smccc.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <asm/setup.h>
#include <asm/bootm.h>
#include <kaslr.h>
#include <stdlib.h>
#include <power-domain.h>
#include <dt-bindings/power/imx8mp-power.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_IMX_HAB) || defined(CONFIG_AVB_ATX) || defined(CONFIG_IMX_TRUSTY_OS)
struct imx_sec_config_fuse_t const imx_sec_config_fuse = {
	.bank = 1,
	.word = 3,
};
#endif

int timer_init(void)
{
#ifdef CONFIG_SPL_BUILD
	struct sctr_regs *sctr = (struct sctr_regs *)SYSCNT_CTRL_BASE_ADDR;
	unsigned long freq = readl(&sctr->cntfid0);

	/* Update with accurate clock frequency */
	asm volatile("msr cntfrq_el0, %0" : : "r" (freq) : "memory");

	clrsetbits_le32(&sctr->cntcr, SC_CNTCR_FREQ0 | SC_CNTCR_FREQ1,
			SC_CNTCR_FREQ0 | SC_CNTCR_ENABLE | SC_CNTCR_HDBG);
#endif

	gd->arch.tbl = 0;
	gd->arch.tbu = 0;

	return 0;
}

void enable_tzc380(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Enable TZASC and lock setting */
	setbits_le32(&gpr->gpr[10], GPR_TZASC_EN);
	setbits_le32(&gpr->gpr[10], GPR_TZASC_EN_LOCK);

	/*
	 * According to TRM, TZASC_ID_SWAP_BYPASS should be set in
	 * order to avoid AXI Bus errors when GPU is in use
	 */
	setbits_le32(&gpr->gpr[10], GPR_TZASC_ID_SWAP_BYPASS);

	/*
	 * imx8mn and imx8mp implements the lock bit for
	 * TZASC_ID_SWAP_BYPASS, enable it to lock settings
	 */
	setbits_le32(&gpr->gpr[10], GPR_TZASC_ID_SWAP_BYPASS_LOCK);

	/*
	 * set Region 0 attribute to allow secure and non-secure
	 * read/write permission. Found some masters like usb dwc3
	 * controllers can't work with secure memory.
	 */
	writel(0xf0000000, TZASC_BASE_ADDR + 0x108);
}

void set_wdog_reset(struct wdog_regs *wdog)
{
	/*
	 * Output WDOG_B signal to reset external pmic or POR_B decided by
	 * the board design. Without external reset, the peripherals/DDR/
	 * PMIC are not reset, that may cause system working abnormal.
	 * WDZST bit is write-once only bit. Align this bit in kernel,
	 * otherwise kernel code will have no chance to set this bit.
	 */
	setbits_le16(&wdog->wcr, WDOG_WDT_MASK | WDOG_WDZST_MASK);
}

#ifdef CONFIG_ARMV8_PSCI
#define PTE_MAP_NS	PTE_BLOCK_NS
#else
#define PTE_MAP_NS	0
#endif

static struct mm_region imx8m_mem_map[] = {
	{
		/* ROM */
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x100000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE
	}, {
		/* CAAM */
		.virt = 0x100000UL,
		.phys = 0x100000UL,
		.size = 0x8000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* OCRAM_S */
		.virt = 0x180000UL,
		.phys = 0x180000UL,
		.size = 0x8000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE | PTE_MAP_NS
	}, {
		/* TCM */
		.virt = 0x7C0000UL,
		.phys = 0x7C0000UL,
		.size = 0x80000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN | PTE_MAP_NS
	}, {
		/* OCRAM */
		.virt = 0x900000UL,
		.phys = 0x900000UL,
		.size = 0x200000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_OUTER_SHARE | PTE_MAP_NS
	}, {
		/* AIPS */
		.virt = 0xB00000UL,
		.phys = 0xB00000UL,
		.size = 0x3f500000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* DRAM1 */
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = PHYS_SDRAM_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
#ifdef CONFIG_IMX_TRUSTY_OS
			 PTE_BLOCK_INNER_SHARE | PTE_MAP_NS
#else
			 PTE_BLOCK_OUTER_SHARE | PTE_MAP_NS
#endif
#ifdef PHYS_SDRAM_2_SIZE
	}, {
		/* DRAM2 */
		.virt = 0x100000000UL,
		.phys = 0x100000000UL,
		.size = PHYS_SDRAM_2_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
#ifdef CONFIG_IMX_TRUSTY_OS
			 PTE_BLOCK_INNER_SHARE | PTE_MAP_NS
#else
			 PTE_BLOCK_OUTER_SHARE | PTE_MAP_NS
#endif
#endif
	}, {
		/* empty entrie to split table entry 5 if needed when TEEs are used */
		0,
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = imx8m_mem_map;

static unsigned int imx8m_find_dram_entry_in_mem_map(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imx8m_mem_map); i++)
		if (imx8m_mem_map[i].phys == CFG_SYS_SDRAM_BASE)
			return i;

	hang();	/* Entry not found, this must never happen. */
}

void enable_caches(void)
{
	/* If OPTEE runs, remove OPTEE memory from MMU table to avoid speculative prefetch
	 * If OPTEE does not run, still update the MMU table according to dram banks structure
	 * to set correct dram size from board_phys_sdram_size
	 */
	int i = 0;
	/*
	 * please make sure that entry initial value matches
	 * imx8m_mem_map for DRAM1
	 */
	int entry = imx8m_find_dram_entry_in_mem_map();
	u64 attrs = imx8m_mem_map[entry].attrs;

	while (i < CONFIG_NR_DRAM_BANKS &&
	       entry < ARRAY_SIZE(imx8m_mem_map)) {
		if (gd->bd->bi_dram[i].start == 0)
			break;
		imx8m_mem_map[entry].phys = gd->bd->bi_dram[i].start;
		imx8m_mem_map[entry].virt = gd->bd->bi_dram[i].start;
		imx8m_mem_map[entry].size = gd->bd->bi_dram[i].size;
		imx8m_mem_map[entry].attrs = attrs;
		debug("Added memory mapping (%d): %llx %llx\n", entry,
		      imx8m_mem_map[entry].phys, imx8m_mem_map[entry].size);
		i++; entry++;
	}

	icache_enable();
	dcache_enable();
}

__weak int board_phys_sdram_size(phys_size_t *size)
{
	if (!size)
		return -EINVAL;

	*size = PHYS_SDRAM_SIZE;

#ifdef PHYS_SDRAM_2_SIZE
	*size += PHYS_SDRAM_2_SIZE;
#endif
	return 0;
}

int dram_init(void)
{
	phys_size_t sdram_size;
	int ret;

	ret = board_phys_sdram_size(&sdram_size);
	if (ret)
		return ret;

	/* rom_pointer[1] contains the size of TEE occupies */
	if (!IS_ENABLED(CONFIG_ARMV8_PSCI) && !IS_ENABLED(CONFIG_SPL_BUILD) && rom_pointer[1])
		gd->ram_size = sdram_size - rom_pointer[1];
	else
		gd->ram_size = sdram_size;

	return 0;
}

int dram_init_banksize(void)
{
	int bank = 0;
	int ret;
	phys_size_t sdram_size;
	phys_size_t sdram_b1_size, sdram_b2_size;

	ret = board_phys_sdram_size(&sdram_size);
	if (ret)
		return ret;

	/* Bank 1 can't cross over 4GB space */
	if (sdram_size > 0xc0000000) {
		sdram_b1_size = 0xc0000000;
		sdram_b2_size = sdram_size - 0xc0000000;
	} else {
		sdram_b1_size = sdram_size;
		sdram_b2_size = 0;
	}

	gd->bd->bi_dram[bank].start = PHYS_SDRAM;
	if (!IS_ENABLED(CONFIG_ARMV8_PSCI) && !IS_ENABLED(CONFIG_SPL_BUILD) && rom_pointer[1]) {
		phys_addr_t optee_start = (phys_addr_t)rom_pointer[0];
		phys_size_t optee_size = (size_t)rom_pointer[1];

		gd->bd->bi_dram[bank].size = optee_start - gd->bd->bi_dram[bank].start;
		if ((optee_start + optee_size) < (PHYS_SDRAM + sdram_b1_size)) {
			if (++bank >= CONFIG_NR_DRAM_BANKS) {
				puts("CONFIG_NR_DRAM_BANKS is not enough\n");
				return -1;
			}

			gd->bd->bi_dram[bank].start = optee_start + optee_size;
			gd->bd->bi_dram[bank].size = PHYS_SDRAM +
				sdram_b1_size - gd->bd->bi_dram[bank].start;
		}
	} else {
		gd->bd->bi_dram[bank].size = sdram_b1_size;
	}

	if (sdram_b2_size) {
		if (++bank >= CONFIG_NR_DRAM_BANKS) {
			puts("CONFIG_NR_DRAM_BANKS is not enough for SDRAM_2\n");
			return -1;
		}
		gd->bd->bi_dram[bank].start = 0x100000000UL;
		gd->bd->bi_dram[bank].size = sdram_b2_size;
	}

	return 0;
}

phys_size_t get_effective_memsize(void)
{
	int ret;
	phys_size_t sdram_size;
	phys_size_t sdram_b1_size;
	ret = board_phys_sdram_size(&sdram_size);
	if (!ret) {
		/* Bank 1 can't cross over 4GB space */
		if (sdram_size > 0xc0000000) {
			sdram_b1_size = 0xc0000000;
		} else {
			sdram_b1_size = sdram_size;
		}

		if (!IS_ENABLED(CONFIG_ARMV8_PSCI) && !IS_ENABLED(CONFIG_SPL_BUILD) &&
		    rom_pointer[1]) {
			/* We will relocate u-boot to Top of dram1. Tee position has two cases:
			 * 1. At the top of dram1,  Then return the size removed optee size.
			 * 2. In the middle of dram1, return the size of dram1.
			 */
			if ((rom_pointer[0] + rom_pointer[1]) == (PHYS_SDRAM + sdram_b1_size))
				return ((phys_addr_t)rom_pointer[0] - PHYS_SDRAM);
		}

		return sdram_b1_size;
	} else {
		return PHYS_SDRAM_SIZE;
	}
}

static u32 get_cpu_variant_type(u32 type)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;

	u32 value = readl(&fuse->tester4);

	if (type == MXC_CPU_IMX8MQ) {
		if ((value & 0x3) == 0x2)
			return MXC_CPU_IMX8MD;
		else if (value & 0x200000)
			return MXC_CPU_IMX8MQL;

	} else if (type == MXC_CPU_IMX8MM) {
		switch (value & 0x3) {
		case 2:
			if (value & 0x1c0000)
				return MXC_CPU_IMX8MMDL;
			else
				return MXC_CPU_IMX8MMD;
		case 3:
			if (value & 0x1c0000)
				return MXC_CPU_IMX8MMSL;
			else
				return MXC_CPU_IMX8MMS;
		default:
			if (value & 0x1c0000)
				return MXC_CPU_IMX8MML;
			break;
		}
	} else if (type == MXC_CPU_IMX8MN) {
		switch (value & 0x3) {
		case 2:
			if (value & 0x1000000) {
				if (value & 0x10000000)	 /* MIPI DSI */
					return MXC_CPU_IMX8MNUD;
				else
					return MXC_CPU_IMX8MNDL;
			} else {
				return MXC_CPU_IMX8MND;
			}
		case 3:
			if (value & 0x1000000) {
				if (value & 0x10000000)	 /* MIPI DSI */
					return MXC_CPU_IMX8MNUS;
				else
					return MXC_CPU_IMX8MNSL;
			} else {
				return MXC_CPU_IMX8MNS;
			}
		default:
			if (value & 0x1000000) {
				if (value & 0x10000000)	 /* MIPI DSI */
					return MXC_CPU_IMX8MNUQ;
				else
					return MXC_CPU_IMX8MNL;
			}
			break;
		}
	} else if (type == MXC_CPU_IMX8MP) {
		u32 value0 = readl(&fuse->tester3);
		u32 flag = 0;

		/* dual core */
		if ((value0 & 0xc0000) == 0x80000)
			flag |= (1 << 10);

		/* vpu disabled, g1, g2 and vc8000e */
		if ((value0 & 0x43000000) == 0x43000000)
			flag = 1;
		else if ((value0 & 0x40000000) == 0x40000000)
			flag |= (1 << 9);/*vc8000e only*/

		/* npu disabled*/
		if ((value & 0x8) == 0x8)
			flag |= BIT(1);

		/* isp disabled */
		if ((value & 0x3) == 0x3)
			flag |= BIT(2);

		/* gpu disabled */
		if ((value & 0xc0) == 0xc0)
			flag |= BIT(3);

		/* lvds disabled */
		if ((value & 0x180000) == 0x180000)
			flag |= BIT(4);

		/* mipi dsi disabled */
		if ((value & 0x60000) == 0x60000)
			flag |= BIT(5);

		/* mipi csi disabled */
		if ((value & 0x18000) == 0x18000)
			flag |= (1 << 6);

		/* HDMI TX & EARC disabled */
		if ((value & 0xc0200000) == 0xc0200000)
			flag |= (1 << 7);

		/* M7 disabled */
		if ((value0 & 0x200000) == 0x200000)
			flag |= (1 << 8);

		switch (flag) {
		case 0x3f:
			return MXC_CPU_IMX8MPUL;
		case 7:
			return MXC_CPU_IMX8MPL;
		case 2:
			return MXC_CPU_IMX8MP6;
		case 0x400:
			return MXC_CPU_IMX8MPD;
		case 0x7d6:
			return MXC_CPU_IMX8MPDSC;
		case 0x3d6:
			return MXC_CPU_IMX8MPSC;
		case 0x4:
			return MXC_CPU_IMX8MP5;
		case 0x404:
			return MXC_CPU_IMX8MPD2;
		default:
			break;
		}

	}

	return type;
}

u32 get_cpu_rev(void)
{
	struct anamix_pll *ana_pll = (struct anamix_pll *)ANATOP_BASE_ADDR;
	u32 reg = readl(&ana_pll->digprog);
	u32 type = (reg >> 16) & 0xff;
	u32 major_low = (reg >> 8) & 0xff;
	u32 rom_version;

	reg &= 0xff;

	/* iMX8MP */
	if (major_low == 0x43) {
		type = get_cpu_variant_type(MXC_CPU_IMX8MP);
	} else if (major_low == 0x42) {
		/* iMX8MN */
		type = get_cpu_variant_type(MXC_CPU_IMX8MN);
	} else if (major_low == 0x41) {
		type = get_cpu_variant_type(MXC_CPU_IMX8MM);
	} else {
		if (reg == CHIP_REV_1_0) {
			/*
			 * For B0 chip, the DIGPROG is not updated,
			 * it is still TO1.0. we have to check ROM
			 * version or OCOTP_READ_FUSE_DATA.
			 * 0xff0055aa is magic number for B1.
			 */
			if (readl((void __iomem *)(OCOTP_BASE_ADDR + 0x40)) == 0xff0055aa) {
				/*
				 * B2 uses same DIGPROG and OCOTP_READ_FUSE_DATA value with B1,
				 * so have to check ROM to distinguish them
				 */
				rom_version = readl((void __iomem *)ROM_VERSION_B0);
				rom_version &= 0xff;
				if (rom_version == CHIP_REV_2_2)
					reg = CHIP_REV_2_2;
				else
					reg = CHIP_REV_2_1;
			} else {
				rom_version =
					readl((void __iomem *)ROM_VERSION_A0);
				if (rom_version != CHIP_REV_1_0) {
					rom_version = readl((void __iomem *)ROM_VERSION_B0);
					rom_version &= 0xff;
					if (rom_version == CHIP_REV_2_0)
						reg = CHIP_REV_2_0;
				}
			}
		}

		type = get_cpu_variant_type(type);
	}

	return (type << 12) | reg;
}

static void imx_set_wdog_powerdown(bool enable)
{
	struct wdog_regs *wdog1 = (struct wdog_regs *)WDOG1_BASE_ADDR;
	struct wdog_regs *wdog2 = (struct wdog_regs *)WDOG2_BASE_ADDR;
	struct wdog_regs *wdog3 = (struct wdog_regs *)WDOG3_BASE_ADDR;

	/* Write to the PDE (Power Down Enable) bit */
	writew(enable, &wdog1->wmcr);
	writew(enable, &wdog2->wmcr);
	writew(enable, &wdog3->wmcr);
}

static int imx8m_check_clock(void)
{
	struct udevice *dev;
	int ret;

	if (CONFIG_IS_ENABLED(CLK)) {
		ret = uclass_get_device_by_name(UCLASS_CLK,
						"clock-controller@30380000",
						&dev);
		if (ret < 0) {
			printf("Failed to find clock node. Check device tree\n");
			return ret;
		}
	}

	return 0;
}
EVENT_SPY_SIMPLE(EVT_DM_POST_INIT_F, imx8m_check_clock);

static void imx8m_setup_snvs(void)
{
	/* Enable SNVS clock */
	clock_enable(CCGR_SNVS, 1);
	/* Initialize glitch detect */
	writel(SNVS_LPPGDR_INIT, SNVS_BASE_ADDR + SNVS_LPLVDR);
	/* Clear interrupt status */
	writel(0xffffffff, SNVS_BASE_ADDR + SNVS_LPSR);
}

static void imx8m_setup_csu_tzasc(void)
{
	const uintptr_t tzasc_base[4] = {
		0x301f0000, 0x301f0000, 0x301f0000, 0x301f0000
	};
	int i, j;

	if (!IS_ENABLED(CONFIG_ARMV8_PSCI))
		return;

	/* CSU */
	for (i = 0; i < 64; i++)
		writel(0x00ff00ff, (void *)CSU_BASE_ADDR + (4 * i));

	/* TZASC */
	for (j = 0; j < 4; j++) {
		writel(0x77777777, (void *)(tzasc_base[j]));
		writel(0x77777777, (void *)(tzasc_base[j]) + 0x4);
		for (i = 0; i <= 0x10; i += 4)
			writel(0, (void *)(tzasc_base[j]) + 0x40 + i);
	}
}

#if defined(CONFIG_IMX_HAB) && defined(CONFIG_IMX8MQ)
static bool is_hdmi_fused(void) {
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;

	u32 value = readl(&fuse->tester4);

	if (is_imx8mq()) {
		if (value & 0x02000000)
			return true;
	}

	return false;
}

bool is_uid_matched(u64 uid) {
	struct tag_serialnr nr;
	get_board_serial(&nr);

	if (lower_32_bits(uid) == nr.low &&
		upper_32_bits(uid) == nr.high)
		return true;

	return false;
}

static void secure_lockup(void)
{
	if (is_imx8mq() && is_soc_rev(CHIP_REV_2_1) &&
		imx_hab_is_enabled() && !is_hdmi_fused()) {
#ifdef CONFIG_SECURE_STICKY_BITS_LOCKUP
		struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;

		clock_enable(CCGR_OCOTP, 1);
		setbits_le32(&ocotp->sw_sticky, 0x6); /* Lock up field return and SRK revoke */
		writel(0x80000000, &ocotp->scs_set); /* Lock up SCS */
#else
		/* Check the Unique ID, if it is matched with UID config, then allow to leave sticky bits unlocked */
		if (!is_uid_matched(CONFIG_IMX_UNIQUE_ID))
			hang();
#endif
	}
}
#endif

int arch_cpu_init(void)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;

#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
	icache_enable();
#endif

	/*
	 * ROM might disable clock for SCTR,
	 * enable the clock before timer_init.
	 */
	if (IS_ENABLED(CONFIG_SPL_BUILD))
		clock_enable(CCGR_SCTR, 1);
	/*
	 * Init timer at very early state, because sscg pll setting
	 * will use it
	 */
	timer_init();

	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		clock_init();

		if (!IS_ENABLED(CONFIG_IMX_WATCHDOG))
			imx_set_wdog_powerdown(false);

#if defined(CONFIG_IMX_HAB) && defined(CONFIG_IMX8MQ)
		secure_lockup();
#endif
		if (is_imx8md() || is_imx8mmd() || is_imx8mmdl() || is_imx8mms() ||
		    is_imx8mmsl() || is_imx8mnd() || is_imx8mndl() || is_imx8mns() ||
		    is_imx8mnsl() || is_imx8mpd() || is_imx8mnud() || is_imx8mnus()) {
			/* Power down cpu core 1, 2 and 3 for iMX8M Dual core or Single core */
			struct pgc_reg *pgc_core1 = (struct pgc_reg *)(GPC_BASE_ADDR + 0x840);
			struct pgc_reg *pgc_core2 = (struct pgc_reg *)(GPC_BASE_ADDR + 0x880);
			struct pgc_reg *pgc_core3 = (struct pgc_reg *)(GPC_BASE_ADDR + 0x8C0);
			struct gpc_reg *gpc = (struct gpc_reg *)GPC_BASE_ADDR;

			writel(0x1, &pgc_core2->pgcr);
			writel(0x1, &pgc_core3->pgcr);
			if (is_imx8mms() || is_imx8mmsl() || is_imx8mns() || is_imx8mnsl() || is_imx8mnus()) {
				writel(0x1, &pgc_core1->pgcr);
				writel(0xE, &gpc->cpu_pgc_dn_trg);
			} else {
				writel(0xC, &gpc->cpu_pgc_dn_trg);
			}
		}
	}

#if defined(CONFIG_ANDROID_SUPPORT)
	/* Enable RTC */
	writel(0x21, 0x30370038);
#endif

	if (is_imx8mq()) {
		clock_enable(CCGR_OCOTP, 1);
		if (readl(&ocotp->ctrl) & 0x200)
			writel(0x200, &ocotp->ctrl_clr);
	}

	imx8m_setup_snvs();

	imx8m_setup_csu_tzasc();

	return 0;
}

int arch_initr_trap(void)
{
	if (IS_ENABLED(CONFIG_IMX_WATCHDOG))
		imx_set_wdog_powerdown(false);

	return 0;
}

#if defined(CONFIG_IMX8MN) || defined(CONFIG_IMX8MP)
struct rom_api *g_rom_api = (struct rom_api *)0x980;
#endif

#if defined(CONFIG_IMX8M)
#include <spl.h>
int imx8m_detect_secondary_image_boot(void)
{
	u32 *rom_log_addr = (u32 *)0x9e0;
	u32 *rom_log;
	u8 event_id;
	int i, boot_secondary = 0;

	/* If the ROM event log pointer is not valid. */
	if (*rom_log_addr < 0x900000 || *rom_log_addr >= 0xb00000 ||
	    *rom_log_addr & 0x3)
		return -EINVAL;

	/* Parse the ROM event ID version 2 log */
	rom_log = (u32 *)(uintptr_t)(*rom_log_addr);
	for (i = 0; i < 128; i++) {
		event_id = rom_log[i] >> 24;
		switch (event_id) {
		case 0x00: /* End of list */
			return boot_secondary;
		/* Log entries with 1 parameter, skip 1 */
		case 0x80: /* Start to perform the device initialization */
		case 0x81: /* The boot device initialization completes */
		case 0x82: /* Starts to execute boot device driver pre-config */
		case 0x8f: /* The boot device initialization fails */
		case 0x90: /* Start to read data from boot device */
		case 0x91: /* Reading data from boot device completes */
		case 0x9f: /* Reading data from boot device fails */
			i += 1;
			continue;
		/* Log entries with 2 parameters, skip 2 */
		case 0xa0: /* Image authentication result */
		case 0xc0: /* Jump to the boot image soon */
			i += 2;
			continue;
		/* Boot from the secondary boot image */
		case 0x51:
			boot_secondary = 1;
			continue;
		default:
			continue;
		}
	}

	return boot_secondary;
}

int spl_mmc_emmc_boot_partition(struct mmc *mmc)
{
	int part, ret;

	part = default_spl_mmc_emmc_boot_partition(mmc);
	if (part == 0)
		return part;

	ret = imx8m_detect_secondary_image_boot();
	if (ret < 0) {
		printf("Could not get boot partition! Using %d\n", part);
		return part;
	}

	if (ret == 1) {
		/*
		 * Swap the eMMC boot partitions in case there was a
		 * fallback event (i.e. primary image was corrupted
		 * and that corruption was recognized by the BootROM),
		 * so the SPL loads the rest of the U-Boot from the
		 * correct eMMC boot partition, since the BootROM
		 * leaves the boot partition set to the corrupted one.
		 */
		if (part == 1)
			part = 2;
		else if (part == 2)
			part = 1;
	}

	return part;
}

int boot_mode_getprisec(void)
{
	return !!imx8m_detect_secondary_image_boot();
}
#endif

#if defined(CONFIG_IMX8MN) || defined(CONFIG_IMX8MP)
#define IMG_CNTN_SET1_OFFSET	GENMASK(22, 19)
unsigned long arch_spl_mmc_get_uboot_raw_sector(struct mmc *mmc,
						unsigned long raw_sect)
{
	u32 val, offset;

	if (fuse_read(2, 1, &val)) {
		debug("Error reading fuse!\n");
		return raw_sect;
	}

	val = FIELD_GET(IMG_CNTN_SET1_OFFSET, val);
	if (val > 10) {
		debug("Secondary image boot disabled!\n");
		return raw_sect;
	}

	if (val == 0)
		offset = SZ_4M;
	else if (val == 1)
		offset = SZ_2M;
	else if (val == 2)
		offset = SZ_1M;
	else	/* flash.bin offset = 1 MiB * 2^n */
		offset = SZ_1M << val;

	offset /= 512;
	offset -= CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_DATA_PART_OFFSET;

	if (imx8m_detect_secondary_image_boot())
		raw_sect += offset;

	return raw_sect;
}
#endif

bool is_usb_boot(void)
{
	return get_boot_device() == USB_BOOT;
}

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
void get_board_serial(struct tag_serialnr *serialnr)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[0];
	struct fuse_bank0_regs *fuse =
		(struct fuse_bank0_regs *)bank->fuse_regs;

	serialnr->low = fuse->uid_low;
	serialnr->high = fuse->uid_high;
}
#endif

#ifdef CONFIG_OF_SYSTEM_SETUP
bool check_fdt_new_path(void *blob)
{
	const char *soc_path = "/soc@0";
	int nodeoff;

	nodeoff = fdt_path_offset(blob, soc_path);
	if (nodeoff < 0)
		return false;

	return true;
}

static int disable_fdt_nodes(void *blob, const char *const nodes_path[], int size_array)
{
	int i = 0;
	int rc;
	int nodeoff;
	const char *status = "disabled";

	for (i = 0; i < size_array; i++) {
		nodeoff = fdt_path_offset(blob, nodes_path[i]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		debug("Found %s node\n", nodes_path[i]);

add_status:
		rc = fdt_setprop(blob, nodeoff, "status", status, strlen(status) + 1);
		if (rc) {
			if (rc == -FDT_ERR_NOSPACE) {
				rc = fdt_increase_size(blob, 512);
				if (!rc)
					goto add_status;
			}
			printf("Unable to update property %s:%s, err=%s\n",
			       nodes_path[i], "status", fdt_strerror(rc));
		} else {
			printf("Modify %s:%s disabled\n",
			       nodes_path[i], "status");
		}
	}

	return 0;
}

#ifdef CONFIG_IMX8MQ
bool check_dcss_fused(void)
{
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[1];
	struct fuse_bank1_regs *fuse =
		(struct fuse_bank1_regs *)bank->fuse_regs;
	u32 value = readl(&fuse->tester4);

	if (value & 0x4000000)
		return true;

	return false;
}

static int disable_mipi_dsi_nodes(void *blob)
{
	static const char * const nodes_path[] = {
		"/mipi_dsi@30A00000",
		"/mipi_dsi_bridge@30A00000",
		"/dsi_phy@30A00300",
		"/soc@0/bus@30800000/mipi_dsi@30a00000",
		"/soc@0/bus@30800000/dphy@30a00300",
		"/soc@0/bus@30800000/mipi-dsi@30a00000",
	};

	return disable_fdt_nodes(blob, nodes_path, ARRAY_SIZE(nodes_path));
}

static int disable_dcss_nodes(void *blob)
{
	static const char * const nodes_path[] = {
		"/dcss@0x32e00000",
		"/dcss@32e00000",
		"/hdmi@32c00000",
		"/hdmi_cec@32c33800",
		"/hdmi_drm@32c00000",
		"/display-subsystem",
		"/sound-hdmi",
		"/sound-hdmi-arc",
		"/soc@0/bus@32c00000/display-controller@32e00000",
		"/soc@0/bus@32c00000/hdmi@32c00000",
	};

	return disable_fdt_nodes(blob, nodes_path, ARRAY_SIZE(nodes_path));
}

static int check_mipi_dsi_nodes(void *blob)
{
	static const char * const lcdif_path[] = {
		"/lcdif@30320000",
		"/soc@0/bus@30000000/lcdif@30320000",
		"/soc@0/bus@30000000/lcd-controller@30320000"
	};
	static const char * const mipi_dsi_path[] = {
		"/mipi_dsi@30A00000",
		"/soc@0/bus@30800000/mipi_dsi@30a00000"
	};
	static const char * const lcdif_ep_path[] = {
		"/lcdif@30320000/port@0/mipi-dsi-endpoint",
		"/soc@0/bus@30000000/lcdif@30320000/port@0/endpoint",
		"/soc@0/bus@30000000/lcd-controller@30320000/port@0/endpoint"
	};
	static const char * const mipi_dsi_ep_path[] = {
		"/mipi_dsi@30A00000/port@1/endpoint",
		"/soc@0/bus@30800000/mipi_dsi@30a00000/ports/port@0/endpoint",
		"/soc@0/bus@30800000/mipi-dsi@30a00000/ports/port@0/endpoint@0"
	};

	int lookup_node;
	int nodeoff;
	bool new_path = check_fdt_new_path(blob);
	int i = new_path ? 1 : 0;

	nodeoff = fdt_path_offset(blob, lcdif_path[i]);
	if (nodeoff < 0 || !fdtdec_get_is_enabled(blob, nodeoff)) {
		/*
		 * If can't find lcdif node or lcdif node is disabled,
		 * then disable all mipi dsi, since they only can input
		 * from DCSS
		 */
		return disable_mipi_dsi_nodes(blob);
	}

	nodeoff = fdt_path_offset(blob, mipi_dsi_path[i]);
	if (nodeoff < 0 || !fdtdec_get_is_enabled(blob, nodeoff))
		return 0;

	nodeoff = fdt_path_offset(blob, lcdif_ep_path[i]);
	if (nodeoff < 0) {
		/*
		 * If can't find lcdif endpoint, then disable all mipi dsi,
		 * since they only can input from DCSS
		 */
		return disable_mipi_dsi_nodes(blob);
	}

	lookup_node = fdtdec_lookup_phandle(blob, nodeoff, "remote-endpoint");
	nodeoff = fdt_path_offset(blob, mipi_dsi_ep_path[i]);

	if (nodeoff > 0 && nodeoff == lookup_node)
		return 0;

	return disable_mipi_dsi_nodes(blob);
}
#endif

void board_quiesce_devices(void)
{
#ifdef CONFIG_USB_DWC3
	if (is_usb_boot())
		disconnect_from_pc();
#endif
}

int disable_vpu_nodes(void *blob)
{
	static const char * const nodes_path_8mq[] = {
		"/vpu@38300000",
		"/soc@0/vpu@38300000",
		"/soc@0/video-codec@38300000",
		"/soc@0/video-codec@38310000",
		"/soc@0/blk-ctrl@38320000",
	};

	static const char * const nodes_path_8mm[] = {
		"/vpu_g1@38300000",
		"/vpu_g2@38310000",
		"/vpu_h1@38320000",
		"/soc@0/video-codec@38300000",
		"/soc@0/video-codec@38310000",
		"/soc@0/blk-ctrl@38330000",
	};

	static const char * const nodes_path_8mp[] = {
		"/vpu_g1@38300000",
		"/vpu_g2@38310000",
		"/vpu_vc8000e@38320000",
		"/soc@0/video-codec@38300000",
		"/soc@0/video-codec@38310000",
		"/soc@0/blk-ctrl@38330000",
		"/soc@0/blk-ctl@38330000",
		"/soc@0/bus@30000000/gpc@303a0000/pgc/power-domain@19",
		"/soc@0/bus@30000000/gpc@303a0000/pgc/power-domain@20",
		"/soc@0/bus@30000000/gpc@303a0000/pgc/power-domain@21",
		"/soc@0/bus@30000000/gpc@303a0000/pgc/power-domain@22"
	};

	if (is_imx8mq())
		return disable_fdt_nodes(blob, nodes_path_8mq, ARRAY_SIZE(nodes_path_8mq));
	else if (is_imx8mm())
		return disable_fdt_nodes(blob, nodes_path_8mm, ARRAY_SIZE(nodes_path_8mm));
	else if (is_imx8mpsc() || is_imx8mpdsc()) /*vc8000e only*/
		return disable_fdt_nodes(blob, &nodes_path_8mp[2], 1);
	else if (is_imx8mp())
		return disable_fdt_nodes(blob, nodes_path_8mp, ARRAY_SIZE(nodes_path_8mp));
	else
		return -EPERM;
}

static int remove_phandle_from_node(void *blob, const char *const blkctrl_path,
	const char *name_property, const char * name, const char *phandle_property, const char *cell_name)
{
	int nodeoff, index, ret, j;
	struct fdtdec_phandle_args args;
	u32 arrays[16];
	int cnt, cell_size, list_len, length;
	const char *list, *str_p;
	char *new_list, *end, *new_str_p;

	nodeoff = fdt_path_offset(blob, blkctrl_path);
	if (nodeoff < 0) {
		printf("Fail to get node %s: %d\n", blkctrl_path, nodeoff);
		return nodeoff;
	}

	index = fdt_stringlist_search(blob, nodeoff, name_property, name);
	if (index < 0) {
		printf("Fail to find power domain name %s from %s: %d\n", name, blkctrl_path, index);
		return index;
	}

	ret = fdtdec_parse_phandle_with_args(blob,
				nodeoff, phandle_property, cell_name,
				0, index, &args);
	if (ret < 0) {
		printf("Fail to get power domain %d from %s: %d\n", index, blkctrl_path, ret);
		return ret;
	}

	cnt = fdtdec_get_int_array_count(blob, nodeoff, phandle_property, arrays, 16);
	if (cnt < 0)
		return cnt;

	cell_size = args.args_count + 1;
	memmove((void *)&arrays[cell_size * index], (void *)&arrays[cell_size * (index + 1)],
		(cnt - (index + 1) * cell_size) * sizeof(u32));

	for (j = 0; j < cnt - cell_size; j++) {
		debug("<%u>, ", arrays[j]);
		arrays[j] = cpu_to_fdt32(arrays[j]);
	}
	debug("\n");

	/* remove from phandle list*/
	ret = fdt_setprop(blob, nodeoff, phandle_property, &arrays, (cnt - cell_size) * sizeof(u32));
	if (ret < 0) {
		printf("Fail to fdt_setprop for %s: %d\n", phandle_property, ret);
		return ret;
	}

	/* try to remove the name from name list */
	list = fdt_getprop(blob, nodeoff, name_property, &list_len);
	new_list = (char *)malloc(list_len);
	if (!new_list) {
		printf("Fail to malloc for string list %d\n", list_len);
		return -ENOMEM;
	}
	memcpy(new_list, list, list_len);
	end = new_list + list_len;

	str_p = fdt_stringlist_get(blob, nodeoff, name_property, index, &length);
	new_str_p = new_list + (str_p - list);

	memmove((void *)new_str_p, (void *)new_str_p + length + 1, end - (new_str_p + length + 1));

	ret = fdt_setprop(blob, nodeoff, name_property, new_list, list_len - (length + 1));
	if (ret < 0)
		printf("Fail to fdt_setprop for %s: %d\n", name_property, ret);

	free(new_list);

	return ret;
}

/* Only remove vc8000e*/
int disable_vpu_blk_ctrl_vc8000e(void *blob, char *name)
{
	int ret;

	ret = remove_phandle_from_node(blob, "/soc@0/blk-ctl@38330000",
		"power-domain-names", name, "power-domains", "#power-domain-cells");
	if (!ret)
		printf("removed power-domain %s\n", name);

	ret = remove_phandle_from_node(blob, "/soc@0/blk-ctl@38330000",
		"clock-names", name, "clocks", "#clock-cells");
	if (!ret)
		printf("removed clocks %s\n", name);

	return ret;
}

int disable_media_blk_ctrl_mipi_phy2(void *blob)
{
	int ret, i;

	static const char * const pd_names[] = {
		"mipi-csi2",
		"mipi-dsi2"
	};

	for (i = 0; i < ARRAY_SIZE(pd_names); i++) {
		ret = remove_phandle_from_node(blob, "/soc@0/bus@32c00000/blk-ctrl@32ec0000",
			"power-domain-names", pd_names[i], "power-domains", "#power-domain-cells");
		if (!ret)
			printf("removed power-domain %s\n", pd_names[i]);
	}

	return ret;
}

#ifdef CONFIG_IMX8MN_LOW_DRIVE_MODE
static int low_drive_gpu_freq(void *blob)
{
	static const char *nodes_path_8mn[] = {
		"/gpu@38000000",
		"/soc@0/gpu@38000000"
	};

	int nodeoff, cnt, i, j;
	u32 assignedclks[7];

	for (i = 0; i < ARRAY_SIZE(nodes_path_8mn); i++) {
		nodeoff = fdt_path_offset(blob, nodes_path_8mn[i]);
		if (nodeoff < 0)
			continue;

		cnt = fdtdec_get_int_array_count(blob, nodeoff, "assigned-clock-rates", assignedclks, 7);
		if (cnt < 0)
			return cnt;

		if (cnt != 7)
			printf("Warning: %s, assigned-clock-rates count %d\n", nodes_path_8mn[i], cnt);
		if (cnt < 2)
			return -1;

		assignedclks[cnt - 1] = 200000000;
		assignedclks[cnt - 2] = 200000000;

		for (j = 0; j < cnt; j++) {
			debug("<%u>, ", assignedclks[j]);
			assignedclks[j] = cpu_to_fdt32(assignedclks[j]);
		}
		debug("\n");

		return fdt_setprop(blob, nodeoff, "assigned-clock-rates", &assignedclks, sizeof(assignedclks));
	}

	return -ENOENT;
}
#endif

static bool check_remote_endpoint(void *blob, const char *ep1, const char *ep2)
{
	int lookup_node;
	int nodeoff;

	nodeoff = fdt_path_offset(blob, ep1);
	if (nodeoff) {
		lookup_node = fdtdec_lookup_phandle(blob, nodeoff, "remote-endpoint");
		nodeoff = fdt_path_offset(blob, ep2);

		if (nodeoff > 0 && nodeoff == lookup_node)
			return true;
	}

	return false;
}

int disable_csi_nodes(void *blob)
{
	static const char * const nodes_path_8mp[] = {
		"/soc@0/bus@32c00000/camera/csi@32e40000",
		"/soc@0/bus@32c00000/camera/csi@32e50000"
	};

	return disable_fdt_nodes(blob, nodes_path_8mp, ARRAY_SIZE(nodes_path_8mp));
}

int disable_cm7_nodes(void *blob)
{
	static const char * const nodes_path_8mp[] = {
		"/imx8mp-cm7"
	};

	return disable_fdt_nodes(blob, nodes_path_8mp, ARRAY_SIZE(nodes_path_8mp));
}

int disable_hdmi_lcdif_nodes(void *blob)
{
	int ret;

	static const char * const hdmi_path_8mp[] = {
		"/soc@0/bus@30c00000/hdmi@32fd8000",
		"/soc@0/bus@30c00000/hdmiphy@32fdff00",
		"/soc@0/bus@30c00000/irqsteer@32fc2000",
		"/soc@0/bus@30c00000/hdmi-pai-pvi@32fc4000",
		"/soc@0/bus@32c00000/blk-ctrl@32fc0000",
		"/soc@0/bus@30c00000/spba-bus@30c00000/aud2htx@30cb0000",
		"/soc@0/bus@30c00000/spba-bus@30c00000/xcvr@30cc0000",
		"/sound-hdmi",
		"/sound-xcvr",
	};

	static const char * const lcdif_path_8mp[] = {
		"/soc@0/bus@30c00000/lcd-controller@32fc6000",
	};

	static const char * const lcdif_ep_path_8mp[] = {
		"/soc@0/bus@30c00000/lcd-controller@32fc6000/port@0/endpoint",
	};
	static const char * const hdmi_ep_path_8mp[] = {
		"/soc@0/bus@30c00000/hdmi@32fd8000/port@0/endpoint",
	};

	ret = disable_fdt_nodes(blob, hdmi_path_8mp, ARRAY_SIZE(hdmi_path_8mp));
	if (ret)
		return ret;

	if (check_remote_endpoint(blob, hdmi_ep_path_8mp[0], lcdif_ep_path_8mp[0])) {
		/* Disable lcdif node */
		return disable_fdt_nodes(blob, lcdif_path_8mp, ARRAY_SIZE(lcdif_path_8mp));
	}

	return 0;
}

int disable_dsi_lcdif_nodes(void *blob)
{
	int ret;

	static const char * const dsi_path_8mp[] = {
		"/soc@0/bus@32c00000/mipi_dsi@32e60000"
	};

	static const char * const lcdif_path_8mp[] = {
		"/soc@0/bus@32c00000/lcd-controller@32e80000"
	};

	static const char * const lcdif_ep_path_8mp[] = {
		"/soc@0/bus@32c00000/lcd-controller@32e80000/port@0/endpoint"
	};
	static const char * const dsi_ep_path_8mp[] = {
		"/soc@0/bus@32c00000/mipi_dsi@32e60000/port@0/endpoint"
	};

	ret = disable_fdt_nodes(blob, dsi_path_8mp, ARRAY_SIZE(dsi_path_8mp));
	if (ret)
		return ret;

	if (check_remote_endpoint(blob, dsi_ep_path_8mp[0], lcdif_ep_path_8mp[0])) {
		/* Disable lcdif node */
		return disable_fdt_nodes(blob, lcdif_path_8mp, ARRAY_SIZE(lcdif_path_8mp));
	}

	return 0;
}

int disable_lvds_lcdif_nodes(void *blob)
{
	int ret, i;

	static const char * const ldb_path_8mp[] = {
		"/soc@0/bus@32c00000/ldb@32ec005c",
		"/soc@0/bus@32c00000/phy@32ec0128",
		"/ldb-display-controller",
		"/phy-lvds"
	};

	static const char * const lcdif_path_8mp[] = {
		"/soc@0/bus@32c00000/lcd-controller@32e90000"
	};

	static const char * const lcdif_ep_path_8mp[] = {
		"/soc@0/bus@32c00000/lcd-controller@32e90000/port@0/endpoint@0",
		"/soc@0/bus@32c00000/lcd-controller@32e90000/port@0/endpoint@1",
		"/soc@0/bus@32c00000/lcd-controller@32e90000/port/endpoint@0",
		"/soc@0/bus@32c00000/lcd-controller@32e90000/port/endpoint@1"
	};
	static const char * const ldb_ep_path_8mp[] = {
		"/soc@0/bus@32c00000/ldb@32ec005c/lvds-channel@0/port@0/endpoint",
		"/soc@0/bus@32c00000/ldb@32ec005c/lvds-channel@1/port@0/endpoint",
		"/ldb-display-controller/lvds-channel@0/port@0/endpoint",
		"/ldb-display-controller/lvds-channel@1/port/endpoint"
	};

	ret = disable_fdt_nodes(blob, ldb_path_8mp, ARRAY_SIZE(ldb_path_8mp));
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(ldb_ep_path_8mp); i++) {
		if (check_remote_endpoint(blob, ldb_ep_path_8mp[i], lcdif_ep_path_8mp[i])) {
			/* Disable lcdif node */
			return disable_fdt_nodes(blob, lcdif_path_8mp, ARRAY_SIZE(lcdif_path_8mp));
		}
	}

	return 0;
}

int disable_gpu_nodes(void *blob)
{
	static const char * const nodes_path_8mn[] = {
		"/gpu@38000000",
		"/soc@/gpu@38000000"
	};

	static const char * const nodes_path_8mp[] = {
		"/gpu3d@38000000",
		"/gpu2d@38008000"
	};

	if (is_imx8mp())
		return disable_fdt_nodes(blob, nodes_path_8mp, ARRAY_SIZE(nodes_path_8mp));
	else
		return disable_fdt_nodes(blob, nodes_path_8mn, ARRAY_SIZE(nodes_path_8mn));
}

int disable_npu_nodes(void *blob)
{
	static const char * const nodes_path_8mp[] = {
		"/vipsi@38500000",
		"/soc@0/npu@38500000",
	};

	return disable_fdt_nodes(blob, nodes_path_8mp, ARRAY_SIZE(nodes_path_8mp));
}

int disable_isp_nodes(void *blob)
{
	static const char * const nodes_path_8mp[] = {
		"/soc@0/bus@32c00000/camera/isp@32e10000",
		"/soc@0/bus@32c00000/camera/isp@32e20000"
	};

	return disable_fdt_nodes(blob, nodes_path_8mp, ARRAY_SIZE(nodes_path_8mp));
}

int disable_dsp_nodes(void *blob)
{
	static const char * const nodes_path_8mp[] = {
		"/dsp@3b6e8000"
	};

	return disable_fdt_nodes(blob, nodes_path_8mp, ARRAY_SIZE(nodes_path_8mp));
}

static void disable_thermal_cpu_nodes(void *blob, u32 disabled_cores)
{
	static const char * const thermal_path[] = {
		"/thermal-zones/cpu-thermal/cooling-maps/map0"
	};

	int nodeoff, cnt, i, ret, j;
	u32 cooling_dev[12];

	for (i = 0; i < ARRAY_SIZE(thermal_path); i++) {
		nodeoff = fdt_path_offset(blob, thermal_path[i]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		cnt = fdtdec_get_int_array_count(blob, nodeoff, "cooling-device", cooling_dev, 12);
		if (cnt < 0)
			continue;

		if (cnt != 12)
			printf("Warning: %s, cooling-device count %d\n", thermal_path[i], cnt);

		for (j = 0; j < cnt; j++)
			cooling_dev[j] = cpu_to_fdt32(cooling_dev[j]);

		ret = fdt_setprop(blob, nodeoff, "cooling-device", &cooling_dev,
				  sizeof(u32) * (12 - disabled_cores * 3));
		if (ret < 0) {
			printf("Warning: %s, cooling-device setprop failed %d\n",
			       thermal_path[i], ret);
			continue;
		}

		printf("Update node %s, cooling-device prop\n", thermal_path[i]);
	}
}

static void disable_pmu_cpu_nodes(void *blob, u32 disabled_cores)
{
	static const char * const pmu_path[] = {
		"/pmu"
	};

	int nodeoff, cnt, i, ret, j;
	u32 irq_affinity[4];

	for (i = 0; i < ARRAY_SIZE(pmu_path); i++) {
		nodeoff = fdt_path_offset(blob, pmu_path[i]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		cnt = fdtdec_get_int_array_count(blob, nodeoff, "interrupt-affinity",
						 irq_affinity, 4);
		if (cnt < 0)
			continue;

		if (cnt != 4)
			printf("Warning: %s, interrupt-affinity count %d\n", pmu_path[i], cnt);

		for (j = 0; j < cnt; j++)
			irq_affinity[j] = cpu_to_fdt32(irq_affinity[j]);

		ret = fdt_setprop(blob, nodeoff, "interrupt-affinity", &irq_affinity,
				 sizeof(u32) * (4 - disabled_cores));
		if (ret < 0) {
			printf("Warning: %s, interrupt-affinity setprop failed %d\n",
			       pmu_path[i], ret);
			continue;
		}

		printf("Update node %s, interrupt-affinity prop\n", pmu_path[i]);
	}
}

static int disable_cpu_nodes(void *blob, u32 disabled_cores)
{
	static const char * const nodes_path[] = {
		"/cpus/cpu@1",
		"/cpus/cpu@2",
		"/cpus/cpu@3",
	};
	u32 i = 0;
	int rc;
	int nodeoff;

	if (disabled_cores > 3)
		return -EINVAL;

	i = 3 - disabled_cores;

	for (; i < 3; i++) {
		nodeoff = fdt_path_offset(blob, nodes_path[i]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		debug("Found %s node\n", nodes_path[i]);

		rc = fdt_del_node(blob, nodeoff);
		if (rc < 0) {
			printf("Unable to delete node %s, err=%s\n",
			       nodes_path[i], fdt_strerror(rc));
		} else {
			printf("Delete node %s\n", nodes_path[i]);
		}
	}

	disable_thermal_cpu_nodes(blob, disabled_cores);
	disable_pmu_cpu_nodes(blob, disabled_cores);

	return 0;
}

static int delete_u_boot_nodes(void *blob)
{
	static const char * const nodes_path = "/u-boot-node";
	int nodeoff;
	int rc;

	nodeoff = fdt_path_offset(blob, nodes_path);
	if (nodeoff < 0)
		return 0;

	debug("Found %s node\n", nodes_path);

	rc = fdt_del_node(blob, nodeoff);
	if (rc < 0) {
		printf("Unable to delete node %s, err=%s\n",
		       nodes_path, fdt_strerror(rc));
	} else {
		printf("Delete node %s\n", nodes_path);
	}

	return 0;
}

static int cleanup_nodes_for_efi(void *blob)
{
	static const char * const path[][2] = {
		{ "/soc@0/bus@32c00000/usb@32e40000", "extcon" },
		{ "/soc@0/bus@32c00000/usb@32e50000", "extcon" },
		{ "/soc@0/bus@30800000/ethernet@30be0000", "phy-reset-gpios" },
		{ "/soc@0/bus@30800000/ethernet@30bf0000", "phy-reset-gpios" }
	};
	int nodeoff, i, rc;

	for (i = 0; i < ARRAY_SIZE(path); i++) {
		nodeoff = fdt_path_offset(blob, path[i][0]);
		if (nodeoff < 0)
			continue; /* Not found, skip it */
		debug("Found %s node\n", path[i][0]);

		rc = fdt_delprop(blob, nodeoff, path[i][1]);
		if (rc == -FDT_ERR_NOTFOUND)
			continue;
		if (rc) {
			printf("Unable to update property %s:%s, err=%s\n",
			       path[i][0], path[i][1], fdt_strerror(rc));
			return rc;
		}

		printf("Remove %s:%s\n", path[i][0], path[i][1]);
	}

	return 0;
}

static int fixup_thermal_trips(void *blob, const char *name)
{
	int minc, maxc;
	int node, trip;

	node = fdt_path_offset(blob, "/thermal-zones");
	if (node < 0)
		return node;

	node = fdt_subnode_offset(blob, node, name);
	if (node < 0)
		return node;

	node = fdt_subnode_offset(blob, node, "trips");
	if (node < 0)
		return node;

	get_cpu_temp_grade(&minc, &maxc);

	fdt_for_each_subnode(trip, blob, node) {
		const char *type;
		int temp, ret;

		type = fdt_getprop(blob, trip, "type", NULL);
		if (!type)
			continue;

		temp = 0;
		if (!strcmp(type, "critical"))
			temp = 1000 * maxc;
		else if (!strcmp(type, "passive"))
			temp = 1000 * (maxc - 10);
		if (temp) {
			ret = fdt_setprop_u32(blob, trip, "temperature", temp);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int ft_system_setup(void *blob, struct bd_info *bd)
{
#ifdef CONFIG_IMX8MQ
	int i = 0;
	int rc;
	int nodeoff;

	if (get_boot_device() == USB_BOOT) {
		disable_dcss_nodes(blob);

		bool new_path = check_fdt_new_path(blob);
		int v = new_path ? 1 : 0;
		static const char * const usb_dwc3_path[] = {
			"/usb@38100000/dwc3",
			"/soc@0/usb@38100000"
		};

		nodeoff = fdt_path_offset(blob, usb_dwc3_path[v]);
		if (nodeoff >= 0) {
			const char *speed = "high-speed";

			debug("Found %s node\n", usb_dwc3_path[v]);

usb_modify_speed:

			rc = fdt_setprop(blob, nodeoff, "maximum-speed", speed, strlen(speed) + 1);
			if (rc) {
				if (rc == -FDT_ERR_NOSPACE) {
					rc = fdt_increase_size(blob, 512);
					if (!rc)
						goto usb_modify_speed;
				}
				printf("Unable to set property %s:%s, err=%s\n",
				       usb_dwc3_path[v], "maximum-speed", fdt_strerror(rc));
			} else {
				printf("Modify %s:%s = %s\n",
				       usb_dwc3_path[v], "maximum-speed", speed);
			}
		} else {
			printf("Can't found %s node\n", usb_dwc3_path[v]);
		}
	}

	/* Disable the CPU idle for A0 chip since the HW does not support it */
	if (is_soc_rev(CHIP_REV_1_0)) {
		static const char * const nodes_path[] = {
			"/cpus/cpu@0",
			"/cpus/cpu@1",
			"/cpus/cpu@2",
			"/cpus/cpu@3",
		};

		for (i = 0; i < ARRAY_SIZE(nodes_path); i++) {
			nodeoff = fdt_path_offset(blob, nodes_path[i]);
			if (nodeoff < 0)
				continue; /* Not found, skip it */

			debug("Found %s node\n", nodes_path[i]);

			rc = fdt_delprop(blob, nodeoff, "cpu-idle-states");
			if (rc == -FDT_ERR_NOTFOUND)
				continue;
			if (rc) {
				printf("Unable to update property %s:%s, err=%s\n",
				       nodes_path[i], "status", fdt_strerror(rc));
				return rc;
			}

			debug("Remove %s:%s\n", nodes_path[i],
			       "cpu-idle-states");
		}
	}

	if (is_imx8mql()) {
		disable_vpu_nodes(blob);
		if (check_dcss_fused()) {
			printf("DCSS is fused\n");
			disable_dcss_nodes(blob);
			check_mipi_dsi_nodes(blob);
		}
	}

	if (is_imx8md())
		disable_cpu_nodes(blob, 2);

#elif defined(CONFIG_IMX8MM)
	if (is_imx8mml() || is_imx8mmdl() ||  is_imx8mmsl())
		disable_vpu_nodes(blob);

	if (is_imx8mmd() || is_imx8mmdl())
		disable_cpu_nodes(blob, 2);
	else if (is_imx8mms() || is_imx8mmsl())
		disable_cpu_nodes(blob, 3);

#elif defined(CONFIG_IMX8MN)
	if (is_imx8mnl() || is_imx8mndl() ||  is_imx8mnsl())
		disable_gpu_nodes(blob);
#ifdef CONFIG_IMX8MN_LOW_DRIVE_MODE
	else {
		int ldm_gpu = low_drive_gpu_freq(blob);

		if (ldm_gpu < 0)
			printf("Update GPU node assigned-clock-rates failed\n");
		else
			printf("Update GPU node assigned-clock-rates ok\n");
	}
#endif

	if (is_imx8mnd() || is_imx8mndl() || is_imx8mnud())
		disable_cpu_nodes(blob, 2);
	else if (is_imx8mns() || is_imx8mnsl() || is_imx8mnus())
		disable_cpu_nodes(blob, 3);

#elif defined(CONFIG_IMX8MP)
	if (is_imx8mpul()) {
		/* Disable GPU */
		disable_gpu_nodes(blob);

		/* Disable DSI */
		disable_dsi_lcdif_nodes(blob);

		/* Disable LVDS */
		disable_lvds_lcdif_nodes(blob);
	}

	if (is_imx8mpsc() || is_imx8mpdsc()) {
		/* Disable CSI */
		disable_csi_nodes(blob);

		/* Disable LVDS */
		disable_lvds_lcdif_nodes(blob);

		/* Disable CM7 */
		disable_cm7_nodes(blob);

		/* Disable HDMI */
		disable_hdmi_lcdif_nodes(blob);

		/* remove pgc for vc8000e only */
		disable_vpu_blk_ctrl_vc8000e(blob, "h1"); /* Pre 6.12 */

		disable_vpu_blk_ctrl_vc8000e(blob, "vc8000e"); /* 6.12+ */

		/* remove pgc for mipi csi2 only */
		disable_media_blk_ctrl_mipi_phy2(blob);
	}

	if (is_imx8mpul() || is_imx8mpl() ||
		is_imx8mpsc() || is_imx8mpdsc())
		disable_vpu_nodes(blob);

	if (is_imx8mpul() || is_imx8mpl() || is_imx8mp6() ||
		is_imx8mpsc() || is_imx8mpdsc())
		disable_npu_nodes(blob);

	if (is_imx8mpul() || is_imx8mpl() ||
		is_imx8mpsc() || is_imx8mpdsc() ||
		is_imx8mpd2() || is_imx8mp5())
		disable_isp_nodes(blob);

	if (is_imx8mpul() || is_imx8mpl() || is_imx8mp6() ||
		is_imx8mpsc() || is_imx8mpdsc() ||
		is_imx8mpd2() || is_imx8mp5())
		disable_dsp_nodes(blob);

	if (is_imx8mpd() || is_imx8mpdsc() || is_imx8mpd2())
		disable_cpu_nodes(blob, 2);
#endif

	cleanup_nodes_for_efi(blob);

	delete_u_boot_nodes(blob);

	if (fixup_thermal_trips(blob, "cpu-thermal"))
		printf("Failed to update cpu-thermal trip(s)");
	if (IS_ENABLED(CONFIG_IMX8MP) &&
	    fixup_thermal_trips(blob, "soc-thermal"))
		printf("Failed to update soc-thermal trip(s)");

	if (IS_ENABLED(CONFIG_KASLR)) {
       int ret = do_generate_kaslr(blob);
       if (ret)
           printf("Unable to set property %s, err=%s\n",
                       "kaslr-seed", fdt_strerror(ret));
	}

#if defined(CONFIG_ANDROID_SUPPORT) || defined(CONFIG_ANDROID_AUTO_SUPPORT)
	return 0;
#else
	return ft_add_optee_node(blob, bd);
#endif
}
#endif

#ifdef CONFIG_OF_BOARD_FIXUP
#ifndef CONFIG_SPL_BUILD
int board_fix_fdt(void *fdt)
{
	if (is_imx8mpul()) {
		int i = 0;
		int nodeoff, ret;
		const char *status = "disabled";
		static const char * const dsi_nodes[] = {
			"/soc@0/bus@32c00000/mipi_dsi@32e60000",
			"/soc@0/bus@32c00000/lcd-controller@32e80000",
			"/dsi-host"
		};

		for (i = 0; i < ARRAY_SIZE(dsi_nodes); i++) {
			nodeoff = fdt_path_offset(fdt, dsi_nodes[i]);
			if (nodeoff > 0) {
set_status:
				ret = fdt_setprop(fdt, nodeoff, "status", status,
						  strlen(status) + 1);
				if (ret == -FDT_ERR_NOSPACE) {
					ret = fdt_increase_size(fdt, 512);
					if (!ret)
						goto set_status;
				}
			}
		}
	}

	return 0;
}
#endif
#endif

#if !CONFIG_IS_ENABLED(SYSRESET)
void reset_cpu(void)
{
	struct watchdog_regs *wdog = (struct watchdog_regs *)WDOG1_BASE_ADDR;

	/* Clear WDA to trigger WDOG_B immediately */
	writew((SET_WCR_WT(1) | WCR_WDT | WCR_WDE | WCR_SRS), &wdog->wcr);

	while (1) {
		/*
		 * spin for .5 seconds before reset
		 */
	}
}
#endif

#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
#if !defined(CONFIG_IMX_TRUSTY_OS) || defined(CONFIG_SPL_BUILD)
	if (IS_ENABLED(CONFIG_FSL_CAAM)) {
		struct udevice *dev;
		int ret;

		ret = uclass_get_device_by_driver(UCLASS_MISC, DM_DRIVER_GET(caam_jr), &dev);
		if (ret)
			printf("Failed to initialize caam_jr: %d\n", ret);
	}
#endif

	return 0;
}
#endif

#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_IMX8MP
#define HSIO_GPR_BASE                               (0x32F10000U)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT    (1)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN          (0x1U << HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT)
#endif

static uint32_t gpc_pu_m_core_offset[11] = {
	0xc00, 0xc40, 0xc80, 0xcc0,
	0xdc0, 0xe00, 0xe40, 0xe80,
	0xec0, 0xf00, 0xf40,
};

#define PGC_PCR				0

void imx_gpc_set_m_core_pgc(unsigned int offset, bool pdn)
{
	uint32_t val;
	uintptr_t reg = GPC_BASE_ADDR + offset;

	val = readl(reg);
	val &= ~(0x1 << PGC_PCR);

	if(pdn)
		val |= 0x1 << PGC_PCR;
	writel(val, reg);
}

void imx8m_usb_power_domain(uint32_t domain_id, bool on)
{
	uint32_t val;
	uintptr_t reg;

#ifdef CONFIG_IMX8MP
	if (on) {
		/* enable usb clock via hsio gpr */
		reg = readl(HSIO_GPR_BASE);
		reg |= HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN;
		writel(reg, HSIO_GPR_BASE);
	}
#endif

	imx_gpc_set_m_core_pgc(gpc_pu_m_core_offset[domain_id], true);

	reg = GPC_BASE_ADDR + (on ? 0xf8 : 0x104);
	val = 1 << (domain_id > 3 ? (domain_id + 3) : domain_id);
	writel(val, reg);
	while (readl(reg) & val)
		;
	imx_gpc_set_m_core_pgc(gpc_pu_m_core_offset[domain_id], false);
}
#endif

int imx8m_usb_power(int usb_id, bool on)
{
	if (usb_id > 1)
		return -EINVAL;

#ifdef CONFIG_SPL_BUILD
	imx8m_usb_power_domain(2 + usb_id, on);
#else
#if IS_ENABLED(CONFIG_POWER_DOMAIN)
	if (is_imx8mq() || is_imx8mp()) {
		struct power_domain pd;
		if (!power_domain_lookup_name((usb_id == 0)? "power-domain@2": "power-domain@3", &pd)) {
			if (on) {
				if (power_domain_on(&pd)) {
					printf("Error power on usb %d\n", usb_id);
					return -EIO;
				}
			} else {
				if (power_domain_off(&pd)) {
					printf("Error power off usb %d\n", usb_id);
					return -EIO;
				}
			}
		}
	}
#endif
#endif

	return 0;
}

#if defined(CONFIG_SPL_BUILD)
#if defined(CONFIG_IMX8MQ) || defined(CONFIG_IMX8MM) || defined(CONFIG_IMX8MN)
bool serror_need_skip = true;

void do_error(struct pt_regs *pt_regs)
{
	/*
	 * If stack is still in ROM reserved OCRAM not switch to SPL,
	 * it is the ROM SError
	 */
	ulong sp;

	asm volatile("mov %0, sp" : "=r"(sp) : );

	if (serror_need_skip && sp < 0x910000 && sp >= 0x900000) {
		/* Check for ERR050342, imx8mq HDCP enabled parts */
		if (is_imx8mq() && !(readl(OCOTP_BASE_ADDR + 0x450) & 0x08000000)) {
			serror_need_skip = false;
			return; /* Do nothing skip the SError in ROM */
		}

		/* Check for ERR050350, field return mode for imx8mq, mm and mn */
		if (readl(OCOTP_BASE_ADDR + 0x630) & 0x1) {
			serror_need_skip = false;
			return; /* Do nothing skip the SError in ROM */
		}
	}

	efi_restore_gd();
	printf("\"Error\" handler, esr 0x%08lx\n", pt_regs->esr);
	show_regs(pt_regs);
	panic("Resetting CPU ...\n");
}
#endif
#endif

#if defined(CONFIG_IMX8MN) || defined(CONFIG_IMX8MP)
enum env_location arch_env_get_location(enum env_operation op, int prio)
{
	enum boot_device dev = get_boot_device();

	if (prio)
		return ENVL_UNKNOWN;

	switch (dev) {
	case QSPI_BOOT:
	case SPI_NOR_BOOT:
		if (IS_ENABLED(CONFIG_ENV_IS_IN_SPI_FLASH))
			return ENVL_SPI_FLASH;
		return ENVL_NOWHERE;
	case NAND_BOOT:
		if (IS_ENABLED(CONFIG_ENV_IS_IN_NAND))
			return ENVL_NAND;
		return ENVL_NOWHERE;
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
		if (IS_ENABLED(CONFIG_ENV_IS_IN_MMC))
			return ENVL_MMC;
		else if (IS_ENABLED(CONFIG_ENV_IS_IN_EXT4))
			return ENVL_EXT4;
		else if (IS_ENABLED(CONFIG_ENV_IS_IN_FAT))
			return ENVL_FAT;
		return ENVL_NOWHERE;
	default:
		return ENVL_NOWHERE;
	}
}

#endif

#ifdef CONFIG_IMX_BOOTAUX
const struct rproc_att hostmap[] = {
	/* aux core , host core,  size */
	{ 0x00000000, 0x007e0000, 0x00020000 },
	/* OCRAM_S */
	{ 0x00180000, 0x00180000, 0x00008000 },
	/* OCRAM */
	{ 0x00900000, 0x00900000, 0x00020000 },
	/* OCRAM */
	{ 0x00920000, 0x00920000, 0x00020000 },
	/* QSPI Code - alias */
	{ 0x08000000, 0x08000000, 0x08000000 },
	/* DDR (Code) - alias */
	{ 0x10000000, 0x80000000, 0x0FFE0000 },
	/* TCML */
	{ 0x1FFE0000, 0x007E0000, 0x00040000 },
	/* OCRAM_S */
	{ 0x20180000, 0x00180000, 0x00008000 },
	/* OCRAM */
	{ 0x20200000, 0x00900000, 0x00040000 },
	/* DDR (Data) */
	{ 0x40000000, 0x40000000, 0x80000000 },
	{ /* sentinel */ }
};

const struct rproc_att *imx_bootaux_get_hostmap(void)
{
	return hostmap;
}
#endif

#ifdef CONFIG_IMX8MQ
int imx8m_dcss_power_init(void)
{
	/* Enable the display CCGR before power on */
	clock_enable(CCGR_DISPLAY, 1);

	writel(0x0000ffff, 0x303A00EC); /*PGC_CPU_MAPPING */
	setbits_le32(0x303A00F8, 0x1 << 10); /*PU_PGC_SW_PUP_REQ : disp was 10 */

	return 0;
}
#endif
