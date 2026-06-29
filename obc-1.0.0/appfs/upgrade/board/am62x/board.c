/*
 * AM62x Board Configuration
 * 只包含平台差异配置，不包含升级逻辑
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

/* 外部声明 - 分区操作接口在fs/目录实现 */
extern struct obc_syspart_ops block_ops;
extern struct obc_syspart_ops block_nohead_ops;
extern struct obc_syspart_ops ext4_ops;

#if 0
/* AM62x TF卡分区定义 */
static struct obc_syspart am62x_pilot_tf_parts[] = {
    OBC_SYSPART(OBC_SYSPART_LOADER0,   "/dev/obcblock/partition_loader0",   NULL, 0, &block_nohead_ops),
    OBC_SYSPART(OBC_SYSPART_LOADER1,   "/dev/obcblock/partition_loader1",   NULL, 0, &block_nohead_ops),
    OBC_SYSPART(OBC_SYSPART_ATF0,      "/dev/obcblock/partition_atf0",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_ATF1,      "/dev/obcblock/partition_atf1",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_ATF2,      "/dev/obcblock/partition_atf2",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_TEEOS0,    "/dev/obcblock/partition_teeos0",    NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_TEEOS1,    "/dev/obcblock/partition_teeos1",    NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_TEEOS2,    "/dev/obcblock/partition_teeos2",    NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_FDT0,      "/dev/obcblock/partition_fdt0",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_FDT1,      "/dev/obcblock/partition_fdt1",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_FDT2,      "/dev/obcblock/partition_fdt2",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_UBOOT0,    "/dev/obcblock/partition_uboot0",    NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_UBOOT1,    "/dev/obcblock/partition_uboot1",    NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_UBOOT2,    "/dev/obcblock/partition_uboot2",    NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_KERNEL0,   "/dev/obcblock/partition_kernel0",   NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_KERNEL1,   "/dev/obcblock/partition_kernel1",   NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_KERNEL2,   "/dev/obcblock/partition_kernel2",   NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_ROOTFS0,   "/dev/obcblock/partition_rootfs0",   "/obc/rootfs0",   MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_ROOTFS1,   "/dev/obcblock/partition_rootfs1",   "/obc/rootfs1",   MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_ROOTFS2,   "/dev/obcblock/partition_rootfs2",   "/obc/rootfs2",   MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_APPFS0,    "/dev/obcblock/partition_appfs0",    "/obc/appfs0",    MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_APPFS1,    "/dev/obcblock/partition_appfs1",    "/obc/appfs1",    MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_APPFS2,    "/dev/obcblock/partition_appfs2",    "/obc/appfs2",    MS_RDONLY, &ext4_ops),
};
#else
/* AM62x TF卡分区定义 */
static struct obc_syspart am62x_pilot_tf_parts[] = {
    OBC_SYSPART(OBC_SYSPART_LOADER0,   "/dev/mmcblk1p1",      NULL, 0, &block_nohead_ops),
    OBC_SYSPART(OBC_SYSPART_LOADER1,   "/dev/mmcblk1p2",      NULL, 0, &block_nohead_ops),
    OBC_SYSPART(OBC_SYSPART_FDT0,      "/dev/mmcblk1p3",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_FDT1,      "/dev/mmcblk1p4",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_FDT2,      "/dev/mmcblk1p5",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_TEEOS0,    "/dev/mmcblk1p6",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_TEEOS1,    "/dev/mmcblk1p7",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_TEEOS2,    "/dev/mmcblk1p8",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_UBOOT0,    "/dev/mmcblk1p9",      NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_UBOOT1,    "/dev/mmcblk1p10",     NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_UBOOT2,    "/dev/mmcblk1p11",     NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_KERNEL0,   "/dev/mmcblk1p12",     NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_KERNEL1,   "/dev/mmcblk1p13",     NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_KERNEL2,   "/dev/mmcblk1p14",     NULL, 0, &block_ops),
    OBC_SYSPART(OBC_SYSPART_ROOTFS0,   "/dev/mmcblk1p15",     "/obc/rootfs0",   MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_ROOTFS1,   "/dev/mmcblk1p16",     "/obc/rootfs1",   MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_ROOTFS2,   "/dev/mmcblk1p17",     "/obc/rootfs2",   MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_APPFS0,    "/dev/mmcblk1p18",     "/obc/appfs0",    MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_APPFS1,    "/dev/mmcblk1p19",     "/obc/appfs1",    MS_RDONLY, &ext4_ops),
    OBC_SYSPART(OBC_SYSPART_APPFS2,    "/dev/mmcblk1p20",     "/obc/appfs2",    MS_RDONLY, &ext4_ops),
};

#endif
/* AM62x TF卡升级对象映射 - 配置：文件名+使用的操作接口+目标分区 */
static struct upgrade_obj am62x_tf_objs[] = {
    /* 文件名              操作接口                目标分区 */
    UPGRADE_OBJ2("loader-sign.bin",  &block_nohead_ops,  OBC_SYSPART_LOADER0,  OBC_SYSPART_LOADER1),
    UPGRADE_OBJ3("atf-sign.bin",     &block_ops,  OBC_SYSPART_ATF0,     OBC_SYSPART_ATF1,     OBC_SYSPART_ATF2),
    UPGRADE_OBJ3("teeos-sign.bin",   &block_ops,  OBC_SYSPART_TEEOS0,   OBC_SYSPART_TEEOS1,   OBC_SYSPART_TEEOS2),
    UPGRADE_OBJ3("fdt-sign.bin",     &block_ops,  OBC_SYSPART_FDT0,     OBC_SYSPART_FDT1,     OBC_SYSPART_FDT2),
    UPGRADE_OBJ3("uboot-sign.bin",   &block_ops,  OBC_SYSPART_UBOOT0,   OBC_SYSPART_UBOOT1,   OBC_SYSPART_UBOOT2),
    UPGRADE_OBJ3("kernel-sign.bin",  &block_ops,  OBC_SYSPART_KERNEL0,  OBC_SYSPART_KERNEL1,  OBC_SYSPART_KERNEL2),
    UPGRADE_OBJ2("rootfs.ext4",      &ext4_ops,   OBC_SYSPART_ROOTFS0,  OBC_SYSPART_ROOTFS1),
    UPGRADE_OBJ2("appfs.ext4",       &ext4_ops,   OBC_SYSPART_APPFS0,   OBC_SYSPART_APPFS1),
};

/* AM62x系统分区表 */
static struct board_syspart_table am62x_syspart_table[] = {
    OBC_SYSPART_TABLE(AM62X_PILOT_TF_PARTID, am62x_pilot_tf_parts, am62x_tf_objs),
};

/* ============================================================================
 * HAL接口函数 - 只提供查询配置的接口，不包含升级逻辑
 * ============================================================================ */

/**
 * @brief 获取系统分区信息
 */
struct obc_syspart *board_get_syspart(OBC_SYSPART_T part)
{
    int partid_idx = 0;
    struct board_syspart_table *table = &am62x_syspart_table[partid_idx];

    for (int i = 0; i < table->syspart_num; i++) {
        if (table->syspart[i].part == part) {
            return &table->syspart[i];
        }
    }
    return NULL;
}

/**
 * @brief 根据文件名获取升级配置（包含操作接口和分区列表）
 */
struct upgrade_obj *board_get_upgrade_config(const char *filename)
{
    if (!filename) {
        return NULL;
    }

    int partid_idx = 0;
    struct board_syspart_table *table = &am62x_syspart_table[partid_idx];

    for (int i = 0; i < table->obj_num; i++) {
        if (strcmp(filename, table->obj[i].filename) == 0) {
            return &table->obj[i];
        }
    }
    return NULL;
}
