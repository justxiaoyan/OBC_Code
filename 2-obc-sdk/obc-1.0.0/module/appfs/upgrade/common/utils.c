/*
 * Utility Functions
 * 工具函数
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "upgrade.h"

/* 类型名称映射表 */
static const struct {
    upgrade_type_t type;
    const char *name;
} type_name_map[] = {
    { UPGRADE_TYPE_LOADER, "loader" },
    { UPGRADE_TYPE_UBOOT,  "uboot" },
    { UPGRADE_TYPE_FDT,    "fdt" },
    { UPGRADE_TYPE_ATF,    "atf" },
    { UPGRADE_TYPE_TEEOS,  "teeos" },
    { UPGRADE_TYPE_KERNEL, "kernel" },
    { UPGRADE_TYPE_APPFS,  "appfs" },
    { UPGRADE_TYPE_ROOTFS, "rootfs" },
};

/**
 * 升级类型转字符串
 */
const char *upgrade_type_to_string(upgrade_type_t type)
{
    size_t i;
    for (i = 0; i < sizeof(type_name_map) / sizeof(type_name_map[0]); i++) {
        if (type_name_map[i].type == type) {
            return type_name_map[i].name;
        }
    }
    return "unknown";
}

/**
 * 字符串转升级类型
 */
upgrade_type_t upgrade_type_from_string(const char *str)
{
    int i;
    for (i = 0; i < sizeof(type_name_map) / sizeof(type_name_map[0]); i++) {
        if (strcasecmp(type_name_map[i].name, str) == 0) {
            return type_name_map[i].type;
        }
    }
    return UPGRADE_TYPE_MAX;
}

/**
 * 镜像格式转字符串
 */
const char *image_format_to_string(image_format_t format)
{
    switch (format) {
    case IMAGE_FORMAT_RAW:
        return "raw";
    case IMAGE_FORMAT_EXT4:
        return "ext4";
    case IMAGE_FORMAT_EXT4_SPARSE:
        return "ext4-sparse";
    case IMAGE_FORMAT_SQUASHFS:
        return "squashfs";
    case IMAGE_FORMAT_AUTO:
        return "auto";
    default:
        return "unknown";
    }
}

/**
 * 打印分区信息
 */
void print_partition_info(const partition_info_t *info)
{
    const char *media_str;

    if (!info) {
        return;
    }

    switch (info->media) {
    case MEDIA_TYPE_EMMC:
        media_str = "EMMC";
        break;
    case MEDIA_TYPE_SD:
        media_str = "SD";
        break;
    default:
        media_str = "Unknown";
        break;
    }

    printf("Partition Information:\n");
    printf("  Name:     %s\n", info->name);
    printf("  Offset:   0x%08x (%u bytes)\n", info->offset, info->offset);
    printf("  Size:     0x%08x (%u bytes, %.2f MB)\n",
           info->size, info->size, (float)info->size / (1024 * 1024));
    printf("  Media:    %s\n", media_str);
    printf("  Bootable: %s\n", info->bootable ? "Yes" : "No");
}
