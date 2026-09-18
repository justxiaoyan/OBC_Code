







#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

#include <stdint.h>
#include <obc_pack.h>
#if !defined(CONFIG_SOC_K3_AM625)
#include <blk.h>
#endif

#define MAX_PART_NUM        (32)

/* 设备树块设备分区信息 */
typedef enum BOARD_ABILITY_DEV_TYPE
{
    BOARD_ABILITY_DEV_EMMC,
    BOARD_ABILITY_DEV_SD,
    BOARD_ABILITY_DEV_FLASH,
}BOARD_ABILITY_DEV_TYPE_E;

/* 设备树块设备分区信息 */
typedef struct BOARD_ABILITY_BLK_PARTS
{
    char lable[16];
    unsigned int addr;
    unsigned int size;
    unsigned char flag;
}__attribute__((packed))BOARD_ABILITY_BLK_PARTS_T;

/* 块设备信息 */
typedef struct BOARD_ABILITY_BLK
{
    int iRdSize;
    unsigned int def_fdt;
    /* Compatibility fields used by the existing i.MX6ULL backend. */
#if !defined(CONFIG_SOC_K3_AM625)
    BOARD_ABILITY_BLK_PARTS_T stParts[MAX_PART_NUM];
    int iPartCount;
    struct blk_desc *pstBlkDev;
#endif
    BOARD_ABILITY_BLK_PARTS_T stParts_mmc[MAX_PART_NUM];      /* obc-mmc 分区表 */
    BOARD_ABILITY_BLK_PARTS_T stParts_flash[MAX_PART_NUM];    /* obc-flash 分区表 */
    int iPartCount_mmc;                                         /* obc-mmc 分区数量 */
    int iPartCount_flash;                                       /* obc-flash 分区数量 */
}__attribute__((packed))BOARD_ABILITY_BLK_T;

/* 块设备信息 */
typedef struct BOARD_ABILITY_BOOT
{
    unsigned char iBootMedia;                           /* 启动介质 */
    unsigned char iMediaId;                             /* 设备 id */
    unsigned char iPartType;                            /* 主备分区 */
    unsigned int  uiFdtAddr;                            /* 设备树加载位置 */
    unsigned int  uiImageAddr;                          /* 内核加载位置 */
    unsigned int  uiTmpAddr;                            /* 升级临时加载位置 */
}__attribute__((packed))BOARD_ABILITY_BOOT_T;


/* 板级能力集结构体，结构体禁止添加任何指针变量，spl阶段为32位系统，为64位系统，否则会导致内存没有对齐 */
typedef struct BOARD_ABILITY_TABLE
{
    char magic[OBC_MAGIC_LEN];  // 魔数 "OBCFS"
    BOARD_ABILITY_BOOT_T        stBoot;                /* 启动方式相关定义 */
    BOARD_ABILITY_BLK_T         stBlk;                 /* 设备分区信息 */
}__attribute__((packed))BOARD_ABILITY_TABLE_T;


/* 板级配置结构体 */
typedef struct BOARD_CONFIG_TABLE
{
    /*
        board_hw_init:板级硬件初始化
        board_env_init：板级设备树初始化
        board_fdt_init：板级设备树初始化
        board_args_init：板级bootargs参数初始化
    */
    int (*board_hw_init)(BOARD_ABILITY_TABLE_T *);
    int (*board_env_init)(BOARD_ABILITY_TABLE_T *);
    int (*board_fdt_init)(BOARD_ABILITY_TABLE_T *);
    int (*board_args_init)(BOARD_ABILITY_TABLE_T *);
}BOARD_CONFIG_TABLE_T;


extern BOARD_ABILITY_TABLE_T *obc_ability_get(void);

extern int obc_board_init(void);



#endif



