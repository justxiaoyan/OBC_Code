/*
 * RAW Filesystem Writer
 * 原始格式文件系统写入（参考 hatos upgrade）
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include "upgrade.h"
#include "fs_writer.h"

#define BLOCK_SIZE (512 * 1024)  /* 512KB */

/**
 * 通用进度回调
 */
static void report_progress(upgrade_progress_cb_t cb, int progress, const char *msg)
{
    if (cb) {
        cb(progress, msg);
    }
}

/**
 * RAW 格式写入
 */
static int raw_write(const char *image,
                     const partition_info_t *part,
                     uint32_t data_offset,
                     upgrade_progress_cb_t cb)
{
    int src_fd = -1, dst_fd = -1;
    char *buf = NULL;
    uint64_t total_size, written = 0;
    ssize_t n;
    int ret = -1;
    char dev_path[128];

    report_progress(cb, 0, "Opening image file");


    /* 打开源文件 */
    src_fd = open(image, O_RDONLY);
    if (src_fd < 0) {
        fprintf(stderr, "Failed to open image: %s\n", strerror(errno));
        return -errno;
    }


    /* 跳过包头（如果有） */
    if (data_offset > 0) {
        if (lseek(src_fd, data_offset, SEEK_SET) < 0) {
            fprintf(stderr, "Failed to seek: %s\n", strerror(errno));
            close(src_fd);
            return -errno;
        }
    }

    /* 获取数据大小 */
    total_size = lseek(src_fd, 0, SEEK_END);
    total_size -= data_offset;
    lseek(src_fd, data_offset, SEEK_SET);


    if (total_size > part->size) {
        fprintf(stderr, "Image size exceeds partition size\n");
        close(src_fd);
        return -EFBIG;
    }

    /* 获取设备路径 */
    if (partition_get_device_path(part, dev_path, sizeof(dev_path)) < 0) {
        close(src_fd);
        return -EINVAL;
    }


    report_progress(cb, 5, "Opening target device");


    /* 打开目标设备，不使用 O_DIRECT，避免对齐问题 */
    dst_fd = open(dev_path, O_WRONLY);
    if (dst_fd < 0) {
        fprintf(stderr, "Failed to open device %s: %s\n", dev_path, strerror(errno));
        close(src_fd);
        return -errno;
    }


    /* 禁用自动错误恢复，避免触发 reset */
    int flags = 0;
    ioctl(dst_fd, BLKFLSBUF, &flags);  /* 刷新缓冲区 */

    /* 定位到分区偏移 */
    if (lseek(dst_fd, part->offset, SEEK_SET) < 0) {
        fprintf(stderr, "Failed to seek to offset: %s\n", strerror(errno));
        goto cleanup;
    }


    /* 分配缓冲区 */
    buf = malloc(BLOCK_SIZE);
    if (!buf) {
        fprintf(stderr, "Failed to allocate buffer\n");
        goto cleanup;
    }

    report_progress(cb, 10, "Writing image data");

    /* 循环写入 */
    while ((n = read(src_fd, buf, BLOCK_SIZE)) > 0) {
        ssize_t written_now = 0;
        while (written_now < n) {
            ssize_t w = write(dst_fd, buf + written_now, n - written_now);
            if (w < 0) {
                fprintf(stderr, "Write failed: %s\n", strerror(errno));
                goto cleanup;
            }
            written_now += w;
        }
        written += n;

        /* 更新进度（10-90%） */
        int progress = 10 + (int)((written * 80) / total_size);
        report_progress(cb, progress, "Writing");
    }

    if (n < 0) {
        fprintf(stderr, "Read failed: %s\n", strerror(errno));
        goto cleanup;
    }

    /* 同步到磁盘 */
    report_progress(cb, 95, "Syncing to disk");
    fsync(dst_fd);

    report_progress(cb, 100, "Write completed");
    ret = 0;

cleanup:
    if (buf) free(buf);
    if (src_fd >= 0) close(src_fd);
    if (dst_fd >= 0) close(dst_fd);
    return ret;
}

/**
 * RAW 写入器定义
 */
fs_writer_t fs_writer_raw = {
    .name = "raw",
    .format = IMAGE_FORMAT_RAW,
    .write = raw_write,
    .verify = NULL,
};
