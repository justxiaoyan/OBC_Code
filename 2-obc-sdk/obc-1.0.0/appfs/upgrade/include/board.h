/*
 * Board Configuration Interface (Generic)
 * 通用板级配置接口 - 所有平台共用
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/mount.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 分区类型枚举 */
typedef enum {
    AM62X_PILOT_TF_PARTID = 0,
    AM62X_PILOT_EMMC_PARTID,
    AM62X_PILOT_FLASH_PARTID,
} AM62X_PARTID_E;

/* 系统分区枚举 */
typedef enum {
    OBC_SYSPART_NONE         = 0,
    OBC_SYSPART_LOADER0      = 1,
    OBC_SYSPART_LOADER1      = 2,
    OBC_SYSPART_LOADER2      = 3,
    OBC_SYSPART_ATF0         = 4,
    OBC_SYSPART_ATF1         = 5,
    OBC_SYSPART_ATF2         = 6,
    OBC_SYSPART_TEEOS0       = 7,
    OBC_SYSPART_TEEOS1       = 8,
    OBC_SYSPART_TEEOS2       = 9,
    OBC_SYSPART_FDT0         = 10,
    OBC_SYSPART_FDT1         = 11,
    OBC_SYSPART_FDT2         = 12,
    OBC_SYSPART_UBOOT0       = 13,
    OBC_SYSPART_UBOOT1       = 14,
    OBC_SYSPART_UBOOT2       = 15,
    OBC_SYSPART_KERNEL0      = 16,
    OBC_SYSPART_KERNEL1      = 17,
    OBC_SYSPART_KERNEL2      = 18,
    OBC_SYSPART_ROOTFS0      = 19,
    OBC_SYSPART_ROOTFS1      = 20,
    OBC_SYSPART_ROOTFS2      = 21,
    OBC_SYSPART_APPFS0       = 22,
    OBC_SYSPART_APPFS1       = 23,
    OBC_SYSPART_APPFS2       = 24,
} OBC_SYSPART_E;

typedef OBC_SYSPART_E OBC_SYSPART_T;

/* 前置声明 */
struct obc_syspart;
struct obc_syspart_ops;

/* 系统分区结构 - 必须在ops之前定义 */
struct obc_syspart {
    OBC_SYSPART_T part;
    char *dev;
    char *mnt;
    unsigned mountflags;
    struct obc_syspart_ops *ops;
};

/* 分区操作接口 */
struct obc_syspart_ops {
    int (*format)(struct obc_syspart *part);
    bool (*ismounted)(struct obc_syspart *part);
    int (*mount)(struct obc_syspart *part);
    int (*umount)(struct obc_syspart *part);
    int (*sync)(struct obc_syspart *part);
    int (*write_part)(struct obc_syspart *part, char *filename, uint32_t offset, char *buf, int len);
    int (*read_part)(struct obc_syspart *part, char *filename, uint32_t offset, char *buf, int len);
};

#define OBC_SYSPART(p, d, m, f, o) \
    {.part = p, .dev = d, .mnt = m, .mountflags = f, .ops = o}

#define UPGRADE_OBJ_PART_NUM    4

/* 升级对象：文件名+操作接口+目标分区 */
struct upgrade_obj {
    char *filename;                              /* 升级文件名 */
    struct obc_syspart_ops *ops;                 /* 使用的操作接口 */
    uint32_t partnum;                            /* 分区数量 */
    uint32_t parts[UPGRADE_OBJ_PART_NUM];        /* 分区ID列表 */
};

#define UPGRADE_OBJ1(f, o, m) \
    {.filename = f, .ops = o, .partnum = 1, .parts = {m, 0, 0, 0}}

#define UPGRADE_OBJ2(f, o, m, b) \
    {.filename = f, .ops = o, .partnum = 2, .parts = {m, b, 0, 0}}

#define UPGRADE_OBJ3(f, o, m, b, t) \
    {.filename = f, .ops = o, .partnum = 3, .parts = {m, b, t, 0}}

/* 板级分区表 */
struct board_syspart_table {
    char partid;
    struct obc_syspart *syspart;
    int syspart_num;
    struct upgrade_obj *obj;
    int obj_num;
};

typedef struct board_syspart_table board_syspart_table_t;

#define OBC_SYSPART_TABLE(id, parts, objs) \
    { \
        .partid = id, \
        .syspart = parts, \
        .syspart_num = sizeof(parts) / sizeof(parts[0]), \
        .obj = objs, \
        .obj_num = sizeof(objs) / sizeof(objs[0]) \
    }

/* ============================================================================
 * 公共接口 - 只提供配置查询，不包含升级逻辑
 * ============================================================================ */

/**
 * @brief 获取系统分区信息
 */
struct obc_syspart *board_get_syspart(OBC_SYSPART_T part);

/**
 * @brief 根据文件名获取升级配置（包含操作接口和分区列表）
 */
struct upgrade_obj *board_get_upgrade_config(const char *filename);

/**
 * @brief 根据文件名获取分区列表（兼容旧接口）
 */
int board_upgrade_file_to_parts(const char *filename, uint32_t *parts, uint32_t *partnum);

/**
 * @brief 获取分区表总数
 */
int board_get_syspart_table_count(void);

/**
 * @brief 获取指定索引的分区表
 */
struct board_syspart_table *board_get_syspart_table_by_index(int index);

/**
 * @brief 板级初始化
 */
int board_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
