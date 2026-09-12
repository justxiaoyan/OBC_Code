


#ifndef __BOARD_CONFIG_AM62X_H__
#define __BOARD_CONFIG_AM62X_H__


#include <env.h>
#include <common.h>
#include <board_config.h>

#define OBC_PLATFORM_NAME                       "am62x"

#define UBOOT_VERSION_CODE(major, minor, patch) ((major) * 10000 + (minor) * 100 + (patch))
#define UBOOT_VERSION(major, minor, patch) UBOOT_VERSION_CODE(major, minor, patch)
#define UBOOT_VERSION_MAJOR(version) ((version) / 10000)
#define UBOOT_VERSION_MINOR(version) (((version) / 100) % 100)
#define UBOOT_VERSION_PATCH(version) ((version) % 100)

#define CONFIG_AM62X_DEV_EMMC                 (0)
#define CONFIG_AM62X_DEV_SD                   (1)

#define CONFIG_FDT_OBC_MMC_NODE           	"obc-mmc"
#define CONFIG_FDT_OBC_FLASH_NODE           "obc-flash"

#define CONFIG_AM62X_EMMC_DEV_NAME            "mmcblk0"
#define CONFIG_AM62X_SD_DEV_NAME              "mmcblk1"


#define CONFIG_AM62X_STR_KERNEL_ADDR             "0x82000000"
#define CONFIG_AM62X_STR_FDT_ADDR                "0x81000000"
#define CONFIG_AM62X_HEX_KERNEL_ADDR             0x82000000
#define CONFIG_AM62X_HEX_FDT_ADDR                0x81000000

/* 启动共享内存位置 */
#define CONFIG_OBC_SHMEM_ADDR                0x98000000

#define CONFIG_DEFAULT_AM62X_FDT0_DEVADDR        0x00900000
#define CONFIG_DEFAULT_AM62X_FDT1_DEVADDR        0x00980000
#define CONFIG_DEFAULT_AM62X_FDT2_DEVADDR        0x00a00000
#define CONFIG_DEFAULT_AM62X_FDT0_SIZE           0x80000
#define CONFIG_DEFAULT_AM62X_FDT1_SIZE           0x80000
#define CONFIG_DEFAULT_AM62X_FDT2_SIZE           0x80000

#define CONFIG_AM62X_FDT0_DEVADDR                CONFIG_DEFAULT_AM62X_FDT0_DEVADDR
#define CONFIG_AM62X_FDT1_DEVADDR                CONFIG_DEFAULT_AM62X_FDT1_DEVADDR
#define CONFIG_AM62X_FDT2_DEVADDR                CONFIG_DEFAULT_AM62X_FDT2_DEVADDR

#define CONFIG_AM62X_FDT_NAME                   "k3-am625-sk.dtb"


#define CONFIG_AM62X_BOOTARGS_SD                "console=ttyS2,115200n8 earlycon=ns16550a,mmio32,0x02800000 rw ignore_logos root=/dev/mmcblk1p15 rootfstype=ext4 rootwait"
#define CONFIG_AM62X_BOOTARGS_EMMC              "console=ttyS2,115200n8 fbcon=rotate:1 earlyprintk rw cma=64M root=PARTLABEL=rootfs0 rootfstype=ext4 rootwait"



extern BOARD_CONFIG_TABLE_T g_am62x_board;





#endif
