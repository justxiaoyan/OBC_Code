







#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__



#define MAX_PART_NUM        (32)

/* 设备树块设备分区信息 */
typedef enum BOARD_ABILITY_DEV_TYPE
{
    BOARD_ABILITY_DEV_SD,
    BOARD_ABILITY_DEV_EMMC,
    BOARD_ABILITY_DEV_NAND,
}BOARD_ABILITY_DEV_TYPE_E;

/* 设备树块设备分区信息 */
typedef struct BOARD_BLK_PARTITIONS
{
    char lable[16];
    uint32_t addr;
    uint32_t size;
    uint8_t flag;
}BOARD_ABILITY_BLK_PARTS_T;

/* 块设备信息 */
typedef struct BOARD_ABILITY_BLK
{
    int iPartCount;
    BOARD_ABILITY_BLK_PARTS_T stParts[MAX_PART_NUM];
}BOARD_ABILITY_BLK_T;


/* 块设备信息 */
typedef struct BOARD_ABILITY_BOOT
{
    unsigned char iBootMedia;                           /* 启动介质 */
    unsigned char iMediaId;                             /* 设备 id */
    unsigned char iPartType;                            /* 主备分区 */
}BOARD_ABILITY_BOOT_T;


/* 板级能力集结构体 */
typedef struct BOARD_ABILITY_TABLE
{
    BOARD_ABILITY_BOOT_T        stBoot;                /* 启动方式相关定义 */
    BOARD_ABILITY_BLK_T         stPart;                /* 设备分区信息 */
}BOARD_ABILITY_TABLE_T;


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







extern int obc_board_init(void);



#endif






































