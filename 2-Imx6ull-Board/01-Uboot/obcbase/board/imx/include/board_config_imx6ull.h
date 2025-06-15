


#ifndef __BOARD_CONFIG_IMX6ULL_H__
#define __BOARD_CONFIG_IMX6ULL_H__



#include <common.h>
#include <board_config.h>



#define CONFIG_IMX6ULL_DEV_SD                   (0)
#define CONFIG_IMX6ULL_DEV_EMMC                 (1)

#define CONFIG_IMX6ULL_EMMC_HOST_NAME           "obc-emmc"
#define CONFIG_IMX6ULL_EMMC_DEV_NAME            "mmcblk1"


#define CONFIG_IMX6ULL_STR_KERNEL_ADDR             "0x82000000"
#define CONFIG_IMX6ULL_STR_FDT_ADDR                "0x83000000"
#define CONFIG_IMX6ULL_HEX_KERNEL_ADDR             0x82000000
#define CONFIG_IMX6ULL_HEX_FDT_ADDR                0x83000000

#define CONFIG_IMX6ULL_FDT_DEVADDR                 0x00080000

#define CONFIG_IMX6ULL_FDT_NAME                "imx6ull-14x14-evk.dtb"


#define CONFIG_IMX6ULL_BOOTARGS_SD                "console=ttymxc0,115200 root=/dev/mmcblk0p2 rootwait rw"
#define CONFIG_IMX6ULL_BOOTARGS_EMMC           "console=ttymxc0,115200 root=/dev/mmcblk1p5 rootwait rw"



extern BOARD_CONFIG_TABLE_T g_imx6ull_board;





#endif
