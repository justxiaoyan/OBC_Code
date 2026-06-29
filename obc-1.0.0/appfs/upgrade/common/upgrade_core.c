/*
 * Upgrade Main Module - Config-Driven Architecture Only
 * 升级主流程（仅配置驱动架构）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <libgen.h>
#include "upgrade.h"
#include "ems_pkg_header.h"
#include "board.h"

/* 外部操作接口声明 */
extern struct obc_syspart_ops block_ops;
extern struct obc_syspart_ops block_nohead_ops;
extern struct obc_syspart_ops ext4_ops;

/* 流式处理缓冲区大小：4MB */
#define UPGRADE_CHUNK_SIZE (4 * 1024 * 1024)

/**
 * 获取块设备或分区的大小
 * @param dev_path 设备路径，如 /dev/mmcblk1p12
 * @return 设备大小（字节），失败返回 0
 */
static uint64_t get_partition_size(const char *dev_path)
{
    int fd;
    uint64_t size = 0;
    struct stat st;

    if (!dev_path) {
        return 0;
    }

    /* 打开设备 */
    fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Warning: Cannot open %s to get size: %s\n",
                dev_path, strerror(errno));
        return 0;
    }

    /* 检查是否是块设备 */
    if (fstat(fd, &st) == 0) {
        if (S_ISBLK(st.st_mode)) {
            /* 使用 BLKGETSIZE64 获取块设备大小 */
            if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
                fprintf(stderr, "Warning: Cannot get size of %s: %s\n",
                        dev_path, strerror(errno));
                size = 0;
            }
        } else {
            /* 普通文件，使用 stat 大小 */
            size = st.st_size;
        }
    }

    close(fd);
    return size;
}

/**
 * 格式化字节数为易读的字符串
 * @param bytes 字节数
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 */
static void format_size(uint64_t bytes, char *buf, size_t size)
{
    if (bytes >= (1ULL << 30)) {
        snprintf(buf, size, "%.2f GB", (double)bytes / (1ULL << 30));
    } else if (bytes >= (1ULL << 20)) {
        snprintf(buf, size, "%.2f MB", (double)bytes / (1ULL << 20));
    } else if (bytes >= (1ULL << 10)) {
        snprintf(buf, size, "%.2f KB", (double)bytes / (1ULL << 10));
    } else {
        snprintf(buf, size, "%llu bytes", (unsigned long long)bytes);
    }
}

/**
 * 预检查所有分区空间是否足够
 * @param config 升级配置对象
 * @param data_size 需要写入的数据大小
 * @return 0 成功，-ENOSPC 空间不足，-EINVAL 参数错误
 */
static int check_partitions_space(struct upgrade_obj *config, size_t data_size)
{
    uint32_t i;

    if (!config) {
        return -EINVAL;
    }

    printf("\n=== Pre-upgrade Space Check ===\n");

    for (i = 0; i < config->partnum; i++) {
        struct obc_syspart *part = board_get_syspart(config->parts[i]);
        if (!part) {
            fprintf(stderr, "Warning: Partition ID %u not found, skipping space check\n",
                    config->parts[i]);
            continue;
        }

        uint64_t part_size = get_partition_size(part->dev);
        if (part_size > 0) {
            char file_size_str[64], part_size_str[64];
            format_size(data_size, file_size_str, sizeof(file_size_str));
            format_size(part_size, part_size_str, sizeof(part_size_str));

            printf("Partition %s: %s (file: %s)\n",
                   part->dev, part_size_str, file_size_str);

            /* 检查空间是否足够 */
            if (data_size > part_size) {
                char shortage_str[64];
                format_size(data_size - part_size, shortage_str, sizeof(shortage_str));

                fprintf(stderr, "\n");
                fprintf(stderr, "╔═════════════════════════════════════════════════╗\n");
                fprintf(stderr, "║    ERROR: Insufficient Space                    ║\n");
                fprintf(stderr, "╚═════════════════════════════════════════════════╝\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "Partition: %s\n", part->dev);
                fprintf(stderr, "File size: %s (%lu bytes)\n",
                        file_size_str, (unsigned long)data_size);
                fprintf(stderr, "Partition size: %s (%llu bytes)\n",
                        part_size_str, (unsigned long long)part_size);
                fprintf(stderr, "Shortage: %s\n", shortage_str);
                fprintf(stderr, "\n");
                fprintf(stderr, "Please use a smaller image or expand the partition.\n");
                fprintf(stderr, "\n");
                return -ENOSPC;
            }
        } else {
            fprintf(stderr, "Warning: Cannot determine size of %s, skipping space check\n",
                    part->dev);
        }
    }

    printf("✓ All partitions have sufficient space\n\n");
    return 0;
}

/**
 * 流式写入数据到分区
 * @param fd 源文件描述符
 * @param data_offset 源文件数据偏移（从此位置开始读取）
 * @param data_size 需要写入的数据大小
 * @param chunk_buf 缓冲区指针
 * @param chunk_size 缓冲区大小
 * @param part 目标分区
 * @param ops 分区操作接口
 * @return 0 成功，负值失败
 */
static int write_partition_streaming(int fd,
                                     uint32_t data_offset,
                                     size_t data_size,
                                     char *chunk_buf,
                                     size_t chunk_size,
                                     struct obc_syspart *part,
                                     struct obc_syspart_ops *ops)
{
    size_t total_written = 0;
    uint32_t write_offset = 0;
    int last_progress = -1;
    int ret;

    if (!chunk_buf || !part || !ops) {
        return -EINVAL;
    }

    printf("  -> Writing data (%zu bytes) in chunks...\n", data_size);

    /* 重新定位到数据开始位置 */
    if (lseek(fd, data_offset, SEEK_SET) != (off_t)data_offset) {
        fprintf(stderr, "  -> ERROR: Failed to seek to data offset\n");
        return -EIO;
    }

    /* 流式读写循环 */
    while (total_written < data_size) {
        /* 计算本次要读取的大小 */
        size_t to_read = (data_size - total_written < chunk_size) ?
                        (data_size - total_written) : chunk_size;

        /* 读取数据块 */
        ssize_t n = read(fd, chunk_buf, to_read);
        if (n < 0) {
            fprintf(stderr, "  -> ERROR: Read failed: %s\n", strerror(errno));
            return -EIO;
        }
        if (n == 0) {
            fprintf(stderr, "  -> ERROR: Unexpected EOF\n");
            return -EIO;
        }

        /* 写入数据块到分区 */
        ret = ops->write_part(part, NULL, write_offset, chunk_buf, n);
        if (ret < 0) {
            fprintf(stderr, "  -> ERROR: Failed to write partition %s at offset %u\n",
                    part->dev, write_offset);
            return ret;
        }

        total_written += n;
        write_offset += n;

        /* 显示进度（每 5% 更新一次） */
        int progress = (total_written * 100) / data_size;
        if (progress != last_progress && progress % 5 == 0) {
            printf("  -> Progress: %d%% (%zu/%zu bytes)\n",
                   progress, total_written, data_size);
            last_progress = progress;
        }
    }

    printf("  -> Written %zu bytes successfully\n", total_written);
    return 0;
}

/**
 * 配置驱动的通用升级流程（流式处理版本）
 * 使用固定大小缓冲区，支持大文件（256MB+）升级
 */
int upgrade_execute_generic(const char *filename,
                            const char *image_path,
                            UPGRADE_PROGRESS_CB_T cb)
{
    int ret = 0;
    int fd = -1;
    struct stat st;
    char *chunk_buf = NULL;
    struct upgrade_obj *config = NULL;
    uint32_t i;
    uint32_t data_offset = 0;
    size_t data_size = 0;

    if (!filename || !image_path) {
        fprintf(stderr, "Invalid parameters\n");
        return -EINVAL;
    }

    /* 1. 获取升级配置（从board.c） */
    config = board_get_upgrade_config(filename);
    if (!config) {
        fprintf(stderr, "No upgrade config found for: %s\n", filename);
        return -ENOENT;
    }

    /* 2. 打开文件并获取信息 */
    fd = open(image_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", image_path, strerror(errno));
        return -errno;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Failed to stat file: %s\n", strerror(errno));
        close(fd);
        return -errno;
    }

    /* 3. 写入完整文件，由各个 ops 自己决定如何处理签名头 */
    data_offset = 0;  /* 从文件开始位置读取 */
    data_size = st.st_size;  /* 完整文件大小 */

    /* 检查并显示是否有签名头（仅用于信息显示） */
    if (ems_pkg_has_header(image_path)) {
        printf("Detected EMS signature header in file\n");
    }
    printf("File size: %ld bytes, will write complete file\n", st.st_size);

    /* 4. 预检查：验证所有分区空间是否足够 */
    ret = check_partitions_space(config, data_size);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    /* 5. 分配固定大小的缓冲区（4MB），而不是整个文件大小 */
    chunk_buf = malloc(UPGRADE_CHUNK_SIZE);
    if (!chunk_buf) {
        fprintf(stderr, "Failed to allocate chunk buffer: %d bytes\n", UPGRADE_CHUNK_SIZE);
        close(fd);
        return -ENOMEM;
    }

    /* 6. 对每个分区执行流式升级 */
    for (i = 0; i < config->partnum; i++) {
        struct obc_syspart *part = board_get_syspart(config->parts[i]);
        if (!part) {
            fprintf(stderr, "Warning: Partition ID %u not found, skipping\n",
                    config->parts[i]);
            continue;
        }

        printf("\n\n[%u/%u] Upgrading: %s (ID=%u)\n",
               i + 1, config->partnum, part->dev, part->part);

        /* 使用配置中指定的操作接口 */
        struct obc_syspart_ops *ops = config->ops;

        /* 步骤1: 卸载（如果是文件系统） */
        if (ops->umount && ops->ismounted) {
            if (ops->ismounted(part)) {
                printf("  -> Unmounting...\n");
                ret = ops->umount(part);
                if (ret < 0) {
                    fprintf(stderr, "  -> Warning: Unmount failed\n");
                }
            }
        }

        /* 步骤2: 格式化/清零 */
        if (ops->format) {
            printf("  -> Formatting...\n");
            ret = ops->format(part);
            if (ret < 0) {
                fprintf(stderr, "  -> ERROR: Format failed, aborting upgrade\n");
                goto cleanup;
            }
        }

        /* 步骤3: 流式写入数据 */
        ret = write_partition_streaming(fd, data_offset, data_size,
                                        chunk_buf, UPGRADE_CHUNK_SIZE,
                                        part, ops);
        if (ret < 0) {
            goto cleanup;
        }

        /* 步骤4: 同步 */
        if (ops->sync) {
            printf("  -> Syncing...\n");
            ops->sync(part);
        }

        /* 步骤5: 挂载（如果需要且之前已挂载） */
        if (ops->mount && part->mnt) {
            printf("  -> Mounting to %s...\n", part->mnt);
            ret = ops->mount(part);
            if (ret < 0) {
                fprintf(stderr, "  -> Warning: Mount failed\n");
            }
        }

        printf("  -> Partition %s upgraded successfully\n", part->dev);

        /* 全局进度回调 */
        if (cb) {
            int global_progress = (i + 1) * 100 / config->partnum;
            char msg[256];
            snprintf(msg, sizeof(msg), "Partition %u/%u done", i + 1, config->partnum);
            cb(global_progress, msg);
        }
    }

    ret = 0;

cleanup:
    if (chunk_buf) {
        free(chunk_buf);
    }
    if (fd >= 0) {
        close(fd);
    }

    /* 如果升级失败，打印详细的状态信息 */
    if (ret < 0) {
        fprintf(stderr, "\n");
        fprintf(stderr, "╔══════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║         Upgrade Failed - System Status          ║\n");
        fprintf(stderr, "╚══════════════════════════════════════════════════╝\n");
        fprintf(stderr, "\n");
        if (i > 0) {
            fprintf(stderr, "Successfully upgraded partitions (%u/%u):\n", i, config->partnum);
            for (uint32_t j = 0; j < i; j++) {
                struct obc_syspart *p = board_get_syspart(config->parts[j]);
                if (p) {
                    fprintf(stderr, "  [✓] %s\n", p->dev);
                }
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "Failed partition:\n");
        if (i < config->partnum) {
            struct obc_syspart *p = board_get_syspart(config->parts[i]);
            if (p) {
                fprintf(stderr, "  [✗] %s\n", p->dev);
            }
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "⚠️  WARNING: System may be in inconsistent state!\n");
        fprintf(stderr, "Please re-run the upgrade to complete the process.\n");
        fprintf(stderr, "\n");
    }

    return ret;
}

/**
 * 初始化升级上下文（包含完整性验证）
 */
int upgrade_init(UPGRADE_CTX_T *ctx)
{
    EMS_PKG_HEADER_T pkg_header;

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

        /* 验证包头魔数 */
        if (ems_pkg_header_verify(&pkg_header) < 0) {
            fprintf(stderr, "Failed to verify package header\n");
            return -EINVAL;
        }

        /* 验证数据完整性（CRC16） */
        if (ems_pkg_data_verify(ctx->image_path, &pkg_header) < 0) {
            fprintf(stderr, "\nUpgrade aborted due to CRC16 verification failure.\n");
            return -EINVAL;
        }

        /* 保存数据偏移 */
        ctx->pkg_data_offset = sizeof(EMS_PKG_HEADER_T);
    } else {
        ctx->pkg_data_offset = 0;
    }

    return 0;
}

/**
 * 清理升级上下文
 */
void upgrade_cleanup(UPGRADE_CTX_T *ctx)
{
    if (!ctx) {
        return;
    }

    /* 当前无需清理操作 */
}
