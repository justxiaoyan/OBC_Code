/*
 * Package Header Parser
 * 升级包签名头解析（EMSFS 格式）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "ems_pkg_header.h"

/**
 * CRC16 计算（CCITT 标准）
 */
uint16_t ems_calculate_crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    uint32_t i, j;

    for (i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

/**
 * 解析包头
 */
int ems_pkg_header_parse(const char *file, ems_pkg_header_t *header)
{
    int fd;
    ssize_t n;

    if (!file || !header) {
        return -EINVAL;
    }

    fd = open(file, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", file, strerror(errno));
        return -errno;
    }

    /* 读取包头 */
    n = read(fd, header, sizeof(ems_pkg_header_t));
    close(fd);

    if (n < (ssize_t)sizeof(ems_pkg_header_t)) {
        return -EIO;
    }

    return 0;
}

/**
 * 验证包头
 */
int ems_pkg_header_verify(const ems_pkg_header_t *header)
{
    if (!header) {
        return -EINVAL;
    }

    /* 检查魔数 */
    if (memcmp(header->magic, EMS_MAGIC, EMS_MAGIC_LEN) != 0) {
        fprintf(stderr, "Invalid package magic\n");
        return -EINVAL;
    }

    return 0;
}

/**
 * 获取数据偏移
 */
int ems_pkg_get_data_offset(const char *file, uint32_t *offset)
{
    ems_pkg_header_t header;
    int ret;

    ret = ems_pkg_header_parse(file, &header);
    if (ret < 0) {
        return ret;
    }

    ret = ems_pkg_header_verify(&header);
    if (ret < 0) {
        return ret;
    }

    *offset = EMS_HEADER_SIZE;
    return 0;
}

/**
 * 获取数据大小
 */
int ems_pkg_get_data_size(const char *file, uint32_t *size)
{
    ems_pkg_header_t header;
    int ret;

    ret = ems_pkg_header_parse(file, &header);
    if (ret < 0) {
        return ret;
    }

    ret = ems_pkg_header_verify(&header);
    if (ret < 0) {
        return ret;
    }

    *size = header.file_size;
    return 0;
}

/**
 * 检查文件是否有包头
 */
int ems_pkg_has_header(const char *file)
{
    ems_pkg_header_t header;
    int ret;

    ret = ems_pkg_header_parse(file, &header);
    if (ret < 0) {
        return 0;
    }

    ret = ems_pkg_header_verify(&header);
    if (ret < 0) {
        return 0;
    }

    return 1;
}

/**
 * 打印包头信息
 */
void ems_pkg_print_header(const ems_pkg_header_t *header)
{
    if (!header) {
        return;
    }

    printf("EMS Package Header:\n");
    printf("  Magic:          %.6s\n", header->magic);
    printf("  File Size:      %u bytes (%.2f MB)\n",
           header->file_size, (float)header->file_size / (1024 * 1024));
    printf("  CRC16:          0x%04x\n", header->crc16);
    printf("  Head Write:     %u\n", header->head_write_flag);
    printf("  Pack File:      %s\n", header->pack_file);
    printf("  Original File:  %s\n", header->file_name);
}
