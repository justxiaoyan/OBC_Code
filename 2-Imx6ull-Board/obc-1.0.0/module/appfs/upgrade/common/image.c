/*
 * Image Detection and Verification Module
 * 镜像格式检测和校验
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "upgrade.h"

/* 文件魔数定义 */
#define EXT4_MAGIC      0xEF53
#define SPARSE_MAGIC    0xED26FF3A
#define SQUASHFS_MAGIC  0x73717368

/**
 * 检测镜像格式
 */
image_format_t image_detect_format(const char *path)
{
    int fd;
    unsigned char buf[4096];
    uint32_t magic;
    ssize_t n;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return IMAGE_FORMAT_RAW;
    }

    /* 读取文件头 */
    n = read(fd, buf, sizeof(buf));
    if (n < 4) {
        close(fd);
        return IMAGE_FORMAT_RAW;
    }

    /* 检查稀疏镜像魔数（文件开头） */
    memcpy(&magic, buf, 4);
    if (magic == SPARSE_MAGIC) {
        close(fd);
        return IMAGE_FORMAT_EXT4_SPARSE;
    }

    /* 检查 squashfs 魔数（文件开头） */
    if (magic == SQUASHFS_MAGIC) {
        close(fd);
        return IMAGE_FORMAT_SQUASHFS;
    }

    /* 检查 ext4 魔数（偏移 0x438） */
    if (n > 0x438 + 2) {
        uint16_t ext4_magic;
        memcpy(&ext4_magic, buf + 0x438, 2);
        if (ext4_magic == EXT4_MAGIC) {
            close(fd);
            return IMAGE_FORMAT_EXT4;
        }
    }

    close(fd);
    return IMAGE_FORMAT_RAW;
}

/**
 * 验证镜像文件
 */
int image_verify(const char *path, image_format_t format)
{
    struct stat st;

    /* 检查文件是否存在 */
    if (stat(path, &st) < 0) {
        fprintf(stderr, "Image file not found: %s\n", path);
        return -ENOENT;
    }

    /* 检查是否为普通文件 */
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Not a regular file: %s\n", path);
        return -EINVAL;
    }

    /* 检查文件大小 */
    if (st.st_size == 0) {
        fprintf(stderr, "Image file is empty: %s\n", path);
        return -EINVAL;
    }

    /* 如果指定了格式，验证格式是否匹配 */
    if (format != IMAGE_FORMAT_AUTO) {
        image_format_t detected = image_detect_format(path);
        if (detected != format) {
            fprintf(stderr, "Image format mismatch: expected %s, detected %s\n",
                    image_format_to_string(format),
                    image_format_to_string(detected));
            return -EINVAL;
        }
    }

    return 0;
}

/**
 * 获取镜像大小
 */
uint64_t image_get_size(const char *path)
{
    struct stat st;

    if (stat(path, &st) < 0) {
        return 0;
    }

    return (uint64_t)st.st_size;
}
