/*
 * Partition Query Module
 * 通过 ems_module 驱动查询分区信息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "upgrade.h"
#include "../../include/ems_ioctl.h"

#define EMS_DEVICE "/dev/ems_block"
#define MMC_DEVICE_BASE "/dev/mmcblk1"

/* 类型到 IOCTL 命令的映射 */
static const struct {
    upgrade_type_t type;
    unsigned long ioctl_cmd;
} type_cmd_map[] = {
    { UPGRADE_TYPE_LOADER, CMD_GET_LOADER_PART_INFO },
    { UPGRADE_TYPE_UBOOT,  CMD_GET_UBOOT_PART_INFO },
    { UPGRADE_TYPE_FDT,    CMD_GET_FDT_PART_INFO },
    { UPGRADE_TYPE_TEEOS,  CMD_GET_TEEOS_PART_INFO },
    { UPGRADE_TYPE_KERNEL, CMD_GET_KERNEL_PART_INFO },
    { UPGRADE_TYPE_ROOTFS, CMD_GET_ROOTFS_PART_INFO },
};

/**
 * 查询分区信息
 */
int partition_query(upgrade_type_t type, partition_info_t *info)
{
    int fd;
    ems_partition_query_resp_t resp;
    unsigned long cmd = 0;
    size_t i;

    if (!info) {
        return -EINVAL;
    }

    /* 查找对应的 ioctl 命令 */
    for (i = 0; i < sizeof(type_cmd_map) / sizeof(type_cmd_map[0]); i++) {
        if (type_cmd_map[i].type == type) {
            cmd = type_cmd_map[i].ioctl_cmd;
            break;
        }
    }

    if (cmd == 0) {
        fprintf(stderr, "Unsupported upgrade type: %d\n", type);
        return -EINVAL;
    }

    /* 打开驱动设备 */
    fd = open(EMS_DEVICE, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", EMS_DEVICE, strerror(errno));
        return -errno;
    }

    /* 查询分区信息 */
    memset(&resp, 0, sizeof(resp));
    if (ioctl(fd, cmd, &resp) < 0) {
        fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
        close(fd);
        return -errno;
    }

    close(fd);

    /* 检查是否找到分区 */
    if (resp.count == 0) {
        fprintf(stderr, "No partition found for type %d\n", type);
        return -ENOENT;
    }

    /* 使用第一个分区（主分区） */
    strncpy(info->name, resp.parts[0].label, sizeof(info->name) - 1);
    info->offset = resp.parts[0].addr;
    info->size = resp.parts[0].size;
    info->bootable = resp.parts[0].bootable;

    /* 转换介质类型 */
    switch (resp.parts[0].media_type) {
    case EMS_MEDIA_TYPE_EMMC:
        info->media = MEDIA_TYPE_EMMC;
        break;
    case EMS_MEDIA_TYPE_SD:
        info->media = MEDIA_TYPE_SD;
        break;
    default:
        info->media = MEDIA_TYPE_EMMC;
        break;
    }

    return 0;
}

/**
 * 查询所有分区信息
 */
int partition_query_all(upgrade_type_t type, partition_info_t *info, int max_count, int *actual_count)
{
    int fd;
    ems_partition_query_resp_t resp;
    unsigned long cmd = 0;
    int i;

    if (!info || !actual_count) {
        return -EINVAL;
    }

    *actual_count = 0;

    /* 查找对应的 ioctl 命令 */
    for (i = 0; i < sizeof(type_cmd_map) / sizeof(type_cmd_map[0]); i++) {
        if (type_cmd_map[i].type == type) {
            cmd = type_cmd_map[i].ioctl_cmd;
            break;
        }
    }

    if (cmd == 0) {
        fprintf(stderr, "Unsupported upgrade type: %d\n", type);
        return -EINVAL;
    }

    /* 打开驱动设备 */
    fd = open(EMS_DEVICE, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", EMS_DEVICE, strerror(errno));
        return -errno;
    }

    /* 查询分区信息 */
    memset(&resp, 0, sizeof(resp));
    if (ioctl(fd, cmd, &resp) < 0) {
        fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
        close(fd);
        return -errno;
    }

    close(fd);

    /* 复制所有分区信息 */
    int count = (resp.count < (uint32_t)max_count) ? (int)resp.count : max_count;
    for (i = 0; i < count; i++) {
        strncpy(info[i].name, resp.parts[i].label, sizeof(info[i].name) - 1);
        info[i].offset = resp.parts[i].addr;
        info[i].size = resp.parts[i].size;
        info[i].bootable = resp.parts[i].bootable;

        /* 转换介质类型 */
        switch (resp.parts[i].media_type) {
        case EMS_MEDIA_TYPE_EMMC:
            info[i].media = MEDIA_TYPE_EMMC;
            break;
        case EMS_MEDIA_TYPE_SD:
            info[i].media = MEDIA_TYPE_SD;
            break;
        default:
            info[i].media = MEDIA_TYPE_EMMC;
            break;
        }

        /* 获取设备路径 */
        partition_get_device_path(&info[i], info[i].dev_path, sizeof(info[i].dev_path));
    }

    *actual_count = count;
    return 0;
}

int partition_get_device_path(const partition_info_t *info, char *path, size_t len)
{
    if (!info || !path) {
        return -EINVAL;
    }

    /* 对于 TF 卡，使用块设备 */
    snprintf(path, len, "%s", MMC_DEVICE_BASE);

    return 0;
}
