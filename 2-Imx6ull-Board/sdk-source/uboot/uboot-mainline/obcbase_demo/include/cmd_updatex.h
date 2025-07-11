



#ifndef __CMD_UPDATEX_H
#define __CMD_UPDATEX_H

#include <board_config.h>



#define SD_DEV_INDEX                    (0)
#define EMMC_DEV_INDEX                  (1)

#define UPDATEX_LOADE_ADDR              (0x84000000)
#define UPDATEX_WRITE_BLOCK_COUNT       (10)
#define UPDATEX_BLOCK_SIZE              (512)
#define UPDATEX_WRITE_SINGLE_SIZE       (1 * 1024)


#define UBOOT_FILE_NAME                "uboot.bin"
#define KERNEL_FILE_NAME               "zImage"
#define FDT_FILE_NAME                  "fdt.dtb"
#define ROOTFS_FILE_NAME               "rootfs.bin"


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
}UPDATEX_FILE_TYPE_E;

typedef enum UPDATEX_FILE_FOMAT_TYPE
{
    UPDATEX_FILE_FOMAT_TYPE_RAW          = 0,
    UPDATEX_FILE_FOMAT_TYPE_FAT          = 1,
}UPDATEX_FILE_FOMAT_TYPE_E;

typedef struct UPDATEX_FW_FILE_LIST
{
    char name[32];
    unsigned char type;
    unsigned char file_fomat;
    int start_sector;
    unsigned char part_index;
}UPDATEX_FW_FILE_LIST_T;











#endif







