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

/* 升级类型 (与UPDATEX_FILE_TYPE保持一致) */
typedef enum UPGRADE_TYPE
{
    UPGRADE_TYPE_NONE = 0,
    UPGRADE_TYPE_LOADER = 1,
    UPGRADE_TYPE_ATF = 2,
    UPGRADE_TYPE_TEEOS = 3,
    UPGRADE_TYPE_FDT = 4,
    UPGRADE_TYPE_UBOOT = 5,
    UPGRADE_TYPE_KERNEL = 6,
    UPGRADE_TYPE_ROOTFS = 7,
    UPGRADE_TYPE_APPFS = 8,
    UPGRADE_TYPE_MAX
}UPGRADE_TYPE_E;

/* 镜像格式 */
typedef enum IMAGE_FORMAT
{
    IMAGE_FORMAT_RAW = 0,
    IMAGE_FORMAT_EXT4,
    IMAGE_FORMAT_EXT4_SPARSE,
    IMAGE_FORMAT_SQUASHFS,
    IMAGE_FORMAT_AUTO,
}IMAGE_FORMAT_E;

/* 介质类型 */
typedef enum MEDIA_TYPE
{
    MEDIA_TYPE_EMMC = 0,
    MEDIA_TYPE_SD,
    MEDIA_TYPE_NAND,
}MEDIA_TYPE_E;

/* 分区信息 */
typedef struct PARTITION_INFO
{
    char name[32];
    char dev_path[64];
    uint32_t offset;
    uint32_t size;
    MEDIA_TYPE_E media;
    bool bootable;
}PARTITION_INFO_T;

/* 升级上下文 */
typedef struct UPGRADE_CTX
{
    UPGRADE_TYPE_E type;
    IMAGE_FORMAT_E format;
    char image_path[256];
    PARTITION_INFO_T partition;         /* 主分区（用于显示） */
    PARTITION_INFO_T partitions[8];     /* 所有分区数组 */
    int part_count;                     /* 分区数量 */
    bool verify;
    bool force;
    int progress;

    /* 签名包头相关 */
    uint32_t pkg_data_offset;    /* 包头后的数据偏移 */
    uint32_t pkg_data_size;      /* 实际数据大小 */
}UPGRADE_CTX_T;

/* 升级回调函数 */
typedef void (*UPGRADE_PROGRESS_CB_T)(int progress, const char *msg);

/* partition.c */
int upgrade_partition_query(UPGRADE_TYPE_E type, PARTITION_INFO_T *info);
int upgrade_partition_query_all(UPGRADE_TYPE_E type, PARTITION_INFO_T *info, int max_count, int *actual_count);
int upgrade_partition_get_device_path(const PARTITION_INFO_T *info, char *path, size_t len);

/* image.c */
IMAGE_FORMAT_E upgrade_image_detect_format(const char *path);
int upgrade_image_verify(const char *path, IMAGE_FORMAT_E format);
uint64_t upgrade_image_get_size(const char *path);

/* upgrade.c */
int upgrade_init(UPGRADE_CTX_T *ctx);
void upgrade_cleanup(UPGRADE_CTX_T *ctx);

/* 配置驱动的通用升级接口 */
int upgrade_execute_generic(const char *filename,
                            const char *image_path,
                            UPGRADE_PROGRESS_CB_T cb);

/* utils.c */
const char *upgrade_type_to_string(UPGRADE_TYPE_E type);
UPGRADE_TYPE_E upgrade_type_from_string(const char *str);
const char *upgrade_image_format_to_string(IMAGE_FORMAT_E format);
void upgrade_print_partition_info(const PARTITION_INFO_T *info);

#endif /* __UPGRADE_H__ */
