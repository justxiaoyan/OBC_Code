/*
 * AM62x Block Partition Operations
 * 裸分区操作接口实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include "board.h"

/* 签名头大小（512字节） */
#define SIGN_HEADER_SIZE    512

/* Block设备格式化 */
static int block_format(struct obc_syspart *part)
{
    int fd;
    ssize_t ret;
    char zero_buf[4096] = {0};
    off_t total_size = 0;

    if (!part || !part->dev) {
        return -1;
    }

    fd = open(part->dev, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", part->dev, strerror(errno));
        return -1;
    }

    /* 写0清空分区头部（512KB） */
    while (total_size < (512 * 1024)) {  /* 清空前512KB */
        ret = write(fd, zero_buf, sizeof(zero_buf));
        if (ret < 0) {
            close(fd);
            return -1;
        }
        total_size += ret;
    }

    fsync(fd);
    close(fd);

    return 0;
}

/* Block设备挂载状态检查 */
static bool block_ismounted(struct obc_syspart *part)
{
    /* Block设备不需要挂载 */
    (void)part;
    return false;
}

/* Block设备挂载 */
static int block_mount(struct obc_syspart *part)
{
    /* Block设备不需要挂载操作 */
    (void)part;
    return 0;
}

/* Block设备卸载 */
static int block_umount(struct obc_syspart *part)
{
    /* Block设备不需要卸载操作 */
    (void)part;
    return 0;
}

/* Block设备同步 */
static int block_sync(struct obc_syspart *part)
{
    (void)part;
    sync();
    return 0;
}

/* Block设备写分区 */
static int block_write_part(struct obc_syspart *part, char *filename,
                            uint32_t offset, char *buf, int len)
{
    int fd;
    ssize_t ret;
    off_t off_ret;

    (void)filename;

    if (!part || !part->dev || !buf || len <= 0) {
        return -1;
    }

    fd = open(part->dev, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", part->dev, strerror(errno));
        return -1;
    }

    /* 定位到指定偏移 */
    if (offset > 0) {
        off_ret = lseek(fd, offset, SEEK_SET);
        if (off_ret == (off_t)-1) {
            fprintf(stderr, "Failed to seek to offset %u: %s\n", offset, strerror(errno));
            close(fd);
            return -1;
        }
    }

    /* 写入数据 */
    ret = write(fd, buf, len);
    if (ret != len) {
        fprintf(stderr, "Failed to write data: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);

    return ret;
}

/* Block设备读分区 */
static int block_read_part(struct obc_syspart *part, char *filename,
                           uint32_t offset, char *buf, int len)
{
    int fd;
    ssize_t ret;
    off_t off_ret;

    (void)filename;

    if (!part || !part->dev || !buf || len <= 0) {
        return -1;
    }

    fd = open(part->dev, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", part->dev, strerror(errno));
        return -1;
    }

    /* 定位到指定偏移 */
    if (offset > 0) {
        off_ret = lseek(fd, offset, SEEK_SET);
        if (off_ret == (off_t)-1) {
            fprintf(stderr, "Failed to seek to offset %u: %s\n", offset, strerror(errno));
            close(fd);
            return -1;
        }
    }

    /* 读取数据 */
    ret = read(fd, buf, len);
    if (ret < 0) {
        fprintf(stderr, "Failed to read data: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    return ret;
}

/* Block操作接口（标准版） */
struct obc_syspart_ops block_ops = {
    .format     = block_format,
    .ismounted  = block_ismounted,
    .mount      = block_mount,
    .umount     = block_umount,
    .sync       = block_sync,
    .write_part = block_write_part,
    .read_part  = block_read_part,
};

/* ============================================================================
 * Block NoHead 操作接口 - 用于Loader分区，跳过签名头写入
 * ============================================================================ */

/* Block设备写分区（跳过签名头） */
static int block_nohead_write_part(struct obc_syspart *part, char *filename,
                                   uint32_t offset, char *buf, int len)
{
    int fd;
    ssize_t ret;
    off_t off_ret;
    char *data_ptr;
    int data_len;

    (void)filename;

    if (!part || !part->dev || !buf || len <= 0) {
        return -1;
    }

    /* 跳过签名头部（512字节） */
    if (len > SIGN_HEADER_SIZE) {
        data_ptr = buf + SIGN_HEADER_SIZE;
        data_len = len - SIGN_HEADER_SIZE;
        printf("  -> Skipping %d bytes sign header\n", SIGN_HEADER_SIZE);
    } else {
        fprintf(stderr, "Warning: File size (%d) <= sign header size (%d), skipping\n",
                len, SIGN_HEADER_SIZE);
        return 0;  /* 文件太小，跳过 */
    }

    fd = open(part->dev, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", part->dev, strerror(errno));
        return -1;
    }

    /* 定位到指定偏移 */
    if (offset > 0) {
        off_ret = lseek(fd, offset, SEEK_SET);
        if (off_ret == (off_t)-1) {
            fprintf(stderr, "Failed to seek to offset %u: %s\n", offset, strerror(errno));
            close(fd);
            return -1;
        }
    }

    /* 写入数据（跳过签名头） */
    ret = write(fd, data_ptr, data_len);
    if (ret != data_len) {
        fprintf(stderr, "Failed to write data: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);

    printf("  -> Written %d bytes (original: %d, skipped header: %d)\n",
           (int)ret, len, SIGN_HEADER_SIZE);

    return ret;
}

/* Block设备读分区（跳过签名头） */
static int block_nohead_read_part(struct obc_syspart *part, char *filename,
                                  uint32_t offset, char *buf, int len)
{
    /* 读取时也跳过签名头 */
    return block_read_part(part, filename, offset + SIGN_HEADER_SIZE, buf, len);
}

/* Block NoHead操作接口（跳过签名头版本） */
struct obc_syspart_ops block_nohead_ops = {
    .format     = block_format,           /* 格式化相同 */
    .ismounted  = block_ismounted,        /* 挂载检查相同 */
    .mount      = block_mount,            /* 挂载相同 */
    .umount     = block_umount,           /* 卸载相同 */
    .sync       = block_sync,             /* 同步相同 */
    .write_part = block_nohead_write_part, /* ⭐ 跳过签名头写入 */
    .read_part  = block_nohead_read_part,  /* ⭐ 跳过签名头读取 */
};
