



#ifndef __CMD_UPDATEX_H
#define __CMD_UPDATEX_H

#include <board_config.h>
#if defined(CONFIG_SOC_K3_AM625)
#include <board_config_am62x.h>
#elif defined(CONFIG_BOARD_CONFIG_IMX6ULL)
#include <board_config_imx6ull.h>
#endif

#ifndef OBC_PLATFORM_NAME
#error "OBC_PLATFORM_NAME must be defined by the selected board configuration"
#endif


#if defined(CONFIG_SOC_K3_AM625)
#define EMMC_DEV_INDEX                  (0)
#define SD_DEV_INDEX                    (1)
#else
#define SD_DEV_INDEX                    (0)
#define EMMC_DEV_INDEX                  (1)
#endif



#define UPDATEX_LOADE_ADDR              (0x84000000)
#define UPDATEX_WRITE_BLOCK_COUNT       (10)
#define UPDATEX_BLOCK_SIZE              (512)
#define UPDATEX_WRITE_SINGLE_SIZE       (1 * 1024)

#define UPDATEX_IMAGE_NAME(partition)  OBC_PLATFORM_NAME "-" partition ".bin"
#define TEE_FILE_NAME                  UPDATEX_IMAGE_NAME("teeos")
#define FDT_FILE_NAME                  UPDATEX_IMAGE_NAME("fdt")
#define UBOOT_FILE_NAME                UPDATEX_IMAGE_NAME("uboot")
#define ROOTFS_FILE_NAME               UPDATEX_IMAGE_NAME("rootfs")
#define KERNEL_FILE_NAME               UPDATEX_IMAGE_NAME("kernel")
#define LOADER_FILE_NAME               UPDATEX_IMAGE_NAME("loader")

typedef enum UPDATEX_TYPE
{
    UPDATEX_TYPE_NONE        = 0,
    UPDATEX_TYPE_SD          = 1,
    UPDATEX_TYPE_TFTP        = 2,
}UPDATEX_TYPE_E;

typedef enum UPDATEX_FILE_TYPE
{
    UPDATEX_FILE_TYPE_NONE          = 0,
    UPDATEX_FILE_TYPE_UBOOT         = 1,
    UPDATEX_FILE_TYPE_KERNEL        = 2,
    UPDATEX_FILE_TYPE_FDT           = 3,
    UPDATEX_FILE_TYPE_ROOTFS        = 4,
    UPDATEX_FILE_TYPE_TEEOS         = 5,
    UPDATEX_FILE_TYPE_LOADER        = 6,
}UPDATEX_FILE_TYPE_E;

typedef enum UPDATEX_FILE_FOMAT_TYPE
{
    UPDATEX_FILE_FOMAT_TYPE_RAW          = 0,
    UPDATEX_FILE_FOMAT_TYPE_FAT          = 1,
}UPDATEX_FILE_FOMAT_TYPE_E;

typedef struct UPDATEX_FW_FILE_LIST
{
    const char *name;
    unsigned char type;
    unsigned char file_fomat;
    int start_sector;
    unsigned char part_index;
}UPDATEX_FW_FILE_LIST_T;











#endif





