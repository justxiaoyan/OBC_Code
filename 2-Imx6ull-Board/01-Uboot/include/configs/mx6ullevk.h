/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX6UL 14x14 EVK board.
 */
#ifndef __MX6ULLEVK_CONFIG_H
#define __MX6ULLEVK_CONFIG_H

#include <asm/arch/imx-regs.h>
#include <linux/sizes.h>
#include <linux/stringify.h>
#include "mx6_common.h"
#include <asm/mach-imx/gpio.h>

#define PHYS_SDRAM_SIZE	SZ_512M

#define CFG_MXC_UART_BASE		UART1_BASE

/* MMC Configs */
#ifdef CONFIG_FSL_USDHC
#define CFG_SYS_FSL_ESDHC_ADDR	USDHC2_BASE_ADDR
#define CFG_SYS_FSL_USDHC_NUM	2
#endif

/* Fix missing semicolon in write_file_to_mmc */
#define CFG_EXTRA_USER_ENV_SETTINGS \
	"tftp_kernel=1\0" \
	"sector_size=0x200\0" \
	"update_file=sdcard.img\0" \
	"download_file=tftp ${dl_addr} ${dl_file}; " \
		"if test $? -ne 0; then echo Error: Download ${dl_file} failed.; exit 1; fi; " \
		"echo Download ${dl_file} OK.; " \
		"setexpr temp ${filesize} + ${sector_size}; setexpr temp ${temp} - 1; " \
		"if test ${sector_size} -lt ${temp}; then setexpr file_sectors ${temp} / ${sector_size}; " \
		"else setenv file_sectors 1; fi; \0" \
	"write_file_to_mmc=mmc dev ${sddev}; " \
		"if test $? -ne 0; then echo Error: MMC device selection failed.; exit 1; fi; " \
		"mmc write ${write_addr} ${write_offset} ${file_sectors}; " \
		"if test $? -ne 0; then echo Error: MMC write failed at offset ${offset}.; exit 1; fi; " \
		"mmc rescan; \0" \
	"up=setenv dl_addr ${loadaddr}; setenv dl_file ${update_file}; " \
		"run download_file; if test $? -ne 0; then exit 1; fi; " \
		"setenv write_addr ${loadaddr}; setenv write_offset 0; run write_file_to_mmc; \0" \
	"emmcboot=echo Booting from eMMC Card ...; " \
		"mmc dev ${mmcdev}; if test $? -ne 0; then echo Error: MMC device selection failed; exit 1; fi; " \
		"run mmcargs; if test $? -ne 0; then echo Error: Setting MMC args failed; exit 1; fi; " \
		"run findfdt; if test $? -ne 0; then echo Error: FDT not found; exit 1; fi; " \
		"run loadfdt; if test $? -ne 0; then echo Error: Loading FDT failed; exit 1; fi; " \
		"run loadimage; if test $? -ne 0; then echo Error: Loading kernel image failed; exit 1; fi; " \
		"bootz ${loadaddr} - ${fdt_addr}; if test $? -ne 0; then echo Error: Kernel boot failed; exit 1; fi;\0" \
	"sddev=0\0" \
	"sdroot=/dev/mmcblk0p2 rootwait rw\0" \
	"sdargs=setenv bootargs console=${console},${baudrate} root=${sdroot}\0" \
	"sdboot=echo Booting from SD Card ...; " \
		"mmc dev ${sddev}; if test $? -ne 0; then echo Error: SD device selection failed; exit 1; fi; " \
		"run sdargs; if test $? -ne 0; then echo Error: Setting SD args failed; exit 1; fi; " \
		"run findfdt; if test $? -ne 0; then echo Error: FDT not found; exit 1; fi; " \
		"if test ${tftp_kernel} = 1; then " \
			"echo Load kernel image from TFTP Server...; " \
			"setenv dl_addr ${loadaddr}; setenv dl_file ${image}; " \
			"run download_file; if test $? -ne 0; then exit 1; fi; " \
			"setenv dl_addr ${fdt_addr}; setenv dl_file ${fdt_file}; " \
			"run download_file; if test $? -ne 0; then exit 1; fi; " \
		"else " \
			"echo Load kernel image from SD Card...; " \
			"setenv mmcdev ${sddev}; " \
			"run loadfdt; if test $? -ne 0; then echo Error: Loading FDT failed; exit 1; fi; " \
			"run loadimage; if test $? -ne 0; then echo Error: Loading kernel image failed; exit 1; fi; " \
		"fi; " \
		"bootz ${loadaddr} - ${fdt_addr}; if test $? -ne 0; then echo Error: Kernel boot failed; exit 1; fi;\0"


#define CFG_EXTRA_ENV_SETTINGS \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"console=ttymxc0\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=imx6ull-14x14-evk.dtb\0" \
	"fdt_addr=0x83000000\0" \
	"boot_fdt=try\0" \
	"ip_dyn=no\0" \
	"videomode=video=ctfb:x:480,y:272,depth:24,pclk:108695,le:8,ri:4,up:2,lo:4,hs:41,vs:10,sync:0,vmode:0\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=1\0" \
	"mmcroot=/dev/mmcblk1p2 rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=${mmcroot}\0" \
	"loadbootscript=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0" \
		"findfdt="\
			"if test $fdt_file = undefined; then " \
				"if test $board_name = ULZ-EVK && test $board_rev = 14X14; then " \
					"setenv fdt_file imx6ulz-14x14-evk.dtb; fi; " \
				"if test $board_name = EVK && test $board_rev = 14X14; then " \
					"setenv fdt_file imx6ull-14x14-evk.dtb; fi; " \
				"if test $fdt_file = undefined; then " \
					"echo WARNING: Could not determine dtb to use; " \
				"fi; " \
				"echo setenv fdt_file ${fdt_file}; " \
			"else exit 0; " \
			"fi;\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs " \
	"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
		"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${image}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0" \
	CFG_EXTRA_USER_ENV_SETTINGS

/* Miscellaneous configurable options */

/* Physical Memory Map */
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CFG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CFG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CFG_SYS_INIT_RAM_SIZE	IRAM_SIZE

/* environment organization */

#ifdef CONFIG_CMD_NET
#define CFG_FEC_ENET_DEV		1
#endif

#endif
