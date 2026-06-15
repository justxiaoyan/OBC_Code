/*
 * EMS Upgrade Tool - Main Header (Refactored)
 * AM62x User-space Upgrade Utility
 */

#ifndef __UPGRADE_H__
#define __UPGRADE_H__

#include <stdint.h>
#include <stdbool.h>

/* 版本信息 */
#define UPGRADE_VERSION "4.0.0"

/* 升级类型 */
typedef enum {
    UPGRADE_TYPE_LOADER = 0,
    UPGRADE_TYPE_UBOOT,
    UPGRADE_TYPE_FDT,
    UPGRADE_TYPE_ATF,
    UPGRADE_TYPE_TEEOS,
    UPGRADE_TYPE_KERNEL,
    UPGRADE_TYPE_APPFS,
    UPGRADE_TYPE_ROOTFS,
    UPGRADE_TYPE_MAX
} upgrade_type_t;

/* 镜像格式 */
typedef enum {
    IMAGE_FORMAT_RAW = 0,
    IMAGE_FORMAT_EXT4,
    IMAGE_FORMAT_EXT4_SPARSE,
    IMAGE_FORMAT_SQUASHFS,
    IMAGE_FORMAT_AUTO,
} image_format_t;

/* 介质类型 */
typedef enum {
    MEDIA_TYPE_EMMC = 0,
    MEDIA_TYPE_SD,
    MEDIA_TYPE_NAND,
} media_type_t;

/* 分区信息 */
typedef struct {
    char name[32];
    char dev_path[64];
    uint32_t offset;
    uint32_t size;
    media_type_t media;
    bool bootable;
} partition_info_t;

/* 升级上下文 */
typedef struct {
    upgrade_type_t type;
    image_format_t format;
    char image_path[256];
    partition_info_t partition;         /* 主分区（用于显示） */
    partition_info_t partitions[8];     /* 所有分区数组 */
    int part_count;                     /* 分区数量 */
    bool verify;
    bool force;
    int progress;

    /* 签名包头相关 */
    uint32_t pkg_data_offset;    /* 包头后的数据偏移 */
    uint32_t pkg_data_size;      /* 实际数据大小 */
} upgrade_ctx_t;

/* 升级回调函数 */
typedef void (*upgrade_progress_cb_t)(int progress, const char *msg);

/* partition.c */
int partition_query(upgrade_type_t type, partition_info_t *info);
int partition_query_all(upgrade_type_t type, partition_info_t *info, int max_count, int *actual_count);
int partition_get_device_path(const partition_info_t *info, char *path, size_t len);

/* image.c */
image_format_t image_detect_format(const char *path);
int image_verify(const char *path, image_format_t format);
uint64_t image_get_size(const char *path);

/* upgrade.c */
int upgrade_init(upgrade_ctx_t *ctx);
int upgrade_execute(upgrade_ctx_t *ctx, upgrade_progress_cb_t cb);
void upgrade_cleanup(upgrade_ctx_t *ctx);

/* utils.c */
const char *upgrade_type_to_string(upgrade_type_t type);
upgrade_type_t upgrade_type_from_string(const char *str);
const char *image_format_to_string(image_format_t format);
void print_partition_info(const partition_info_t *info);

#endif /* __UPGRADE_H__ */
