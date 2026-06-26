/*
 * Upgrade Main Module - Refactored
 * 升级主流程（重构版，支持签名头）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "upgrade.h"
#include "ems_pkg_header.h"
#include "fs_writer.h"

/**
 * 初始化升级上下文
 */
int upgrade_init(upgrade_ctx_t *ctx)
{
    ems_pkg_header_t pkg_header;
    partition_info_t partitions[8];
    int part_count = 0;

    if (!ctx) {
        return -EINVAL;
    }

    /* 检查是否有签名包头 */
    if (ems_pkg_has_header(ctx->image_path)) {
        /* 解析包头 */
        if (ems_pkg_header_parse(ctx->image_path, &pkg_header) < 0) {
            fprintf(stderr, "Failed to parse package header\n");
            return -EINVAL;
        }

        /* 验证包头 */
        if (ems_pkg_header_verify(&pkg_header) < 0) {
            fprintf(stderr, "Failed to verify package header\n");
            return -EINVAL;
        }

        /* 检查 head_write_flag */
        if (pkg_header.head_write_flag == 1) {
            /* 需要写入包头，从文件开头开始写，包含整个文件 */
            ctx->pkg_data_offset = 0;
            ctx->pkg_data_size = pkg_header.file_size + EMS_HEADER_SIZE;
        } else {
            /* 不写入包头，跳过 512 字节 */
            ctx->pkg_data_offset = EMS_HEADER_SIZE;
            ctx->pkg_data_size = pkg_header.file_size;
        }
    } else {
        /* 无包头，整个文件都是数据 */
        ctx->pkg_data_offset = 0;
        ctx->pkg_data_size = 0;
    }

    /* 检查镜像文件 */
    if (image_verify(ctx->image_path, ctx->format) < 0) {
        return -EINVAL;
    }

    /* 如果是自动检测格式 */
    if (ctx->format == IMAGE_FORMAT_AUTO) {
        ctx->format = image_detect_format(ctx->image_path);
    }

    /* 查询所有分区 */
    if (partition_query_all(ctx->type, partitions, 8, &part_count) < 0) {
        fprintf(stderr, "Failed to query partition info\n");
        return -ENOENT;
    }

    if (part_count == 0) {
        fprintf(stderr, "No partition found for type: %s\n",
                upgrade_type_to_string(ctx->type));
        return -ENOENT;
    }

    /* 使用第一个分区作为主分区（用于显示信息） */
    memcpy(&ctx->partition, &partitions[0], sizeof(partition_info_t));

    /* 获取设备路径 */
    if (partition_get_device_path(&ctx->partition,
                                   ctx->partition.dev_path,
                                   sizeof(ctx->partition.dev_path)) < 0) {
        return -EINVAL;
    }

    /* 检查大小（使用实际数据大小） */
    uint64_t check_size = ctx->pkg_data_size > 0 ?
                          ctx->pkg_data_size :
                          image_get_size(ctx->image_path);

    if (check_size > ctx->partition.size) {
        fprintf(stderr, "Image size (%lu) exceeds partition size (%u)\n",
                check_size, ctx->partition.size);
        if (!ctx->force) {
            return -EFBIG;
        }
        fprintf(stderr, "Force mode enabled, continuing...\n");
    }

    /* 保存所有分区信息 */
    ctx->part_count = part_count;
    for (int i = 0; i < part_count && i < 8; i++) {
        memcpy(&ctx->partitions[i], &partitions[i], sizeof(partition_info_t));
    }

    return 0;
}

/**
 * 执行升级
 */
int upgrade_execute(upgrade_ctx_t *ctx, upgrade_progress_cb_t cb)
{
    fs_writer_t *writer;
    int ret;

    if (!ctx) {
        return -EINVAL;
    }

    /* 获取文件系统写入器 */
    writer = fs_writer_get(ctx->format);
    if (!writer) {
        fprintf(stderr, "No writer found for format: %s\n",
                image_format_to_string(ctx->format));
        return -ENOTSUP;
    }

    printf("\n=============Upgrade Start ============\n");

    /* 依次升级所有分区 */
    for (int i = 0; i < ctx->part_count; i++) {
        partition_info_t *part = &ctx->partitions[i];

        if (i > 0) {
            /* 不是第一个分区，简单显示 */
            printf("Upgrading partition %d/%d: %s (Offset: 0x%08x)\n",
                   i + 1, ctx->part_count, part->name, part->offset);
        } else {
            /* 第一个分区，已经显示过详细信息了 */
            printf("Upgrading partition %d/%d: %s\n",
                   i + 1, ctx->part_count, part->name);
        }

        /* 执行写入 */
        ret = writer->write(ctx->image_path, part, ctx->pkg_data_offset, cb);
        if (ret < 0) {
            fprintf(stderr, "\n✗ Upgrade failed for %s\n", part->name);
            return ret;
        }

        printf("Partition %s upgraded successfully\n\n", part->name);
    }

    if (ctx->part_count > 1) {
        printf("=============Upgrade End ============\n");
    }

    return 0;
}

/**
 * 清理升级上下文
 */
void upgrade_cleanup(upgrade_ctx_t *ctx)
{
    if (!ctx) {
        return;
    }

    /* 当前无需清理操作 */
}
