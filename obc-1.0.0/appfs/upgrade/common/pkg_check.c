/*
 * Package Check & Verification
 * 升级包检查与验证（包头+数据完整性）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "ems_pkg_header.h"
#include "crc16.h"

/**
 * 解析包头
 */
int ems_pkg_header_parse(const char *file, EMS_PKG_HEADER_T *header)
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
    n = read(fd, header, sizeof(EMS_PKG_HEADER_T));
    close(fd);

    if (n < (ssize_t)sizeof(EMS_PKG_HEADER_T)) {
        return -EIO;
    }

    return 0;
}

/**
 * 验证包头（只验证魔数）
 */
int ems_pkg_header_verify(const EMS_PKG_HEADER_T *header)
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
 * 验证包数据完整性（CRC16）
 */
int ems_pkg_data_verify(const char *file, const EMS_PKG_HEADER_T *header)
{
    int fd;
    uint8_t *data = NULL;
    ssize_t n;
    uint16_t calculated_crc;
    int ret = -1;

    if (!file || !header) {
        fprintf(stderr, "Invalid parameters for data verification\n");
        return -EINVAL;
    }

    /* 打开文件 */
    fd = open(file, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", file, strerror(errno));
        return -errno;
    }

    /* 跳过包头（512字节） */
    if (lseek(fd, EMS_HEADER_SIZE, SEEK_SET) != EMS_HEADER_SIZE) {
        fprintf(stderr, "Failed to seek past header\n");
        close(fd);
        return -EIO;
    }

    /* 分配内存读取数据 */
    data = (uint8_t *)malloc(header->file_size);
    if (!data) {
        fprintf(stderr, "Failed to allocate %u bytes for verification\n", header->file_size);
        close(fd);
        return -ENOMEM;
    }

    /* 读取文件数据 */
    n = read(fd, data, header->file_size);
    close(fd);

    if (n != (ssize_t)header->file_size) {
        fprintf(stderr, "Failed to read data: expected %u, got %zd\n",
                header->file_size, n);
        ret = -EIO;
        goto cleanup;
    }

    /* 计算CRC16 */
    calculated_crc = crc16_calculate(data, header->file_size);

    /* 验证CRC16 */
    if (calculated_crc != header->crc16) {
        fprintf(stderr, "\n");
        fprintf(stderr, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        fprintf(stderr, "❌ CRC16 verification FAILED!\n");
        fprintf(stderr, "Expected: 0x%04x\n", header->crc16);
        fprintf(stderr, "Got:      0x%04x\n", calculated_crc);
        fprintf(stderr, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "The upgrade package is corrupted!\n");
        fprintf(stderr, "Please re-download or re-generate the package.\n");
        fprintf(stderr, "\n");
        ret = -EINVAL;
        goto cleanup;
    }

    printf("%s CRC16 verification PASSED\n", header->pack_file);
    ret = 0;

cleanup:
    if (data) {
        free(data);
    }
    return ret;
}

/**
 * 获取数据偏移
 */
int ems_pkg_get_data_offset(const char *file, uint32_t *offset)
{
    EMS_PKG_HEADER_T header;
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
    EMS_PKG_HEADER_T header;
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
    EMS_PKG_HEADER_T header;
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
void ems_pkg_print_header(const EMS_PKG_HEADER_T *header)
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
