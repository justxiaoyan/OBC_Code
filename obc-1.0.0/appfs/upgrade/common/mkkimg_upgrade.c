/*
 * mkkimg Package Upgrader Implementation
 * 实现mkkimg整包升级功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>
#include "mkkimg_upgrade.h"
#include "upgrade.h"
#include "crc16.h"

/* CRC32计算 */
uint32_t crc32_calc(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    size_t i, j;

    for (i = 0; i < length; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }

    return ~crc;
}

/* 从文件类型映射到升级类型 */
static UPGRADE_TYPE_E mkkimg_type_to_upgrade_type(uint32_t mkkimg_type)
{
    /* mkkimg类型值 → UPGRADE_TYPE枚举值映射 */
    switch (mkkimg_type) {
        case 1:  /* UPDATEX_FILE_TYPE_LOADER */
            return UPGRADE_TYPE_LOADER;  /* = 0 */
        case 2:  /* UPDATEX_FILE_TYPE_ATF */
            return UPGRADE_TYPE_ATF;     /* = 1 */
        case 3:  /* UPDATEX_FILE_TYPE_TEEOS */
            return UPGRADE_TYPE_TEEOS;   /* = 2 */
        case 4:  /* UPDATEX_FILE_TYPE_FDT */
            return UPGRADE_TYPE_FDT;     /* = 3 */
        case 5:  /* UPDATEX_FILE_TYPE_UBOOT */
            return UPGRADE_TYPE_UBOOT;   /* = 4 */
        case 6:  /* UPDATEX_FILE_TYPE_KERNEL */
            return UPGRADE_TYPE_KERNEL;  /* = 5 */
        case 7:  /* UPDATEX_FILE_TYPE_ROOTFS */
            return UPGRADE_TYPE_ROOTFS;  /* = 6 */
        case 8:  /* UPDATEX_FILE_TYPE_APPFS */
            return UPGRADE_TYPE_APPFS;   /* = 7 */
        default:
            return UPGRADE_TYPE_NONE;
    }
}

/* 获取文件类型名称 */
static const char *get_type_name(uint32_t type)
{
    switch (type) {
        case UPDATEX_FILE_TYPE_LOADER: return "Loader";
        case UPDATEX_FILE_TYPE_ATF:    return "ATF";
        case UPDATEX_FILE_TYPE_TEEOS:  return "TEE-OS";
        case UPDATEX_FILE_TYPE_FDT:    return "DeviceTree";
        case UPDATEX_FILE_TYPE_UBOOT:  return "U-Boot";
        case UPDATEX_FILE_TYPE_KERNEL: return "Kernel";
        case UPDATEX_FILE_TYPE_ROOTFS: return "RootFS";
        case UPDATEX_FILE_TYPE_APPFS:  return "AppFS";
        default:                       return "Unknown";
    }
}

/**
 * 验证mkkimg镜像完整性
 */
int mkkimg_verify_package(const char *image_path)
{
    FILE *fp = NULL;
    mkkimg_header_t header;
    uint16_t calc_crc16;
    int ret = 0;

    printf("=== Verifying mkkimg package ===\n");

    /* 打开镜像文件 */
    fp = fopen(image_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open %s: %s\n", image_path, strerror(errno));
        return -1;
    }

    /* 读取头部 */
    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(fp);
        return -1;
    }

    /* 验证魔术数 */
    if (header.magic != MKKIMG_MAGIC) {
        fprintf(stderr, "Error: Invalid magic number 0x%08X (expected 0x%08X)\n",
                header.magic, MKKIMG_MAGIC);
        fclose(fp);
        return -1;
    }
    printf("✓ Magic number verified: 0x%08X\n", header.magic);

    /* 验证CRC16 */
    calc_crc16 = crc16_calculate((uint8_t*)&header,
                             offsetof(mkkimg_header_t, package_crc16));
    if (calc_crc16 != header.package_crc16) {
        fprintf(stderr, "Error: CRC16 mismatch! Calculated: 0x%04X, Stored: 0x%04X\n",
                calc_crc16, header.package_crc16);
        fclose(fp);
        return -1;
    }
    printf("✓ Package CRC16 verified: 0x%04X\n", header.package_crc16);

    /* 显示包信息 */
    printf("  File count: %u\n", header.file_count);
    printf("  Total size: %u bytes (%.2f MB)\n",
           header.total_size, header.total_size / (1024.0 * 1024.0));

    /* 验证每个文件的CRC32 */
    printf("\n=== Verifying individual files ===\n");
    for (uint32_t i = 0; i < header.file_count && i < MKKIMG_MAX_FILES; i++) {
        mkkimg_file_info_t *file_info = &header.files[i];
        uint8_t *file_data = NULL;
        uint32_t calc_crc32;

        printf("[%u/%u] %s (%s, %u bytes)...\n",
               i + 1, header.file_count,
               file_info->filename,
               get_type_name(file_info->file_type),
               file_info->size);

        /* 分配缓冲区 */
        file_data = (uint8_t*)malloc(file_info->size);
        if (!file_data) {
            fprintf(stderr, "Error: Cannot allocate memory for %s\n", file_info->filename);
            ret = -1;
            break;
        }

        /* 定位并读取文件数据 */
        if (fseek(fp, file_info->offset, SEEK_SET) != 0) {
            fprintf(stderr, "Error: Cannot seek to offset 0x%08X\n", file_info->offset);
            free(file_data);
            ret = -1;
            break;
        }

        if (fread(file_data, 1, file_info->size, fp) != file_info->size) {
            fprintf(stderr, "Error: Cannot read file data\n");
            free(file_data);
            ret = -1;
            break;
        }

        /* 计算CRC32 */
        calc_crc32 = crc32_calc(file_data, file_info->size);
        if (calc_crc32 != file_info->crc32) {
            fprintf(stderr, "✗ CRC32 mismatch! Calculated: 0x%08X, Stored: 0x%08X\n",
                    calc_crc32, file_info->crc32);
            free(file_data);
            ret = -1;
            break;
        }

        printf("  ✓ CRC32 verified: 0x%08X\n", file_info->crc32);
        free(file_data);
    }

    fclose(fp);

    if (ret == 0) {
        printf("\n✓ Package verification PASSED\n");
    } else {
        printf("\n✗ Package verification FAILED\n");
    }

    return ret;
}

/**
 * 解析并升级mkkimg打包的镜像
 */
int mkkimg_upgrade_package(const char *image_path)
{
    FILE *fp = NULL;
    mkkimg_header_t header;
    int ret = 0;
    char temp_file[256];
    uint32_t success_count = 0;

    printf("\n========================================\n");
    printf("  mkkimg Package Upgrade\n");
    printf("========================================\n");
    printf("Package: %s\n\n", image_path);

    /* 步骤1: 验证整包 */
    ret = mkkimg_verify_package(image_path);
    if (ret < 0) {
        fprintf(stderr, "\nError: Package verification failed, upgrade aborted!\n");
        return -1;
    }

    /* 打开镜像文件 */
    fp = fopen(image_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open %s\n", image_path);
        return -1;
    }

    /* 读取头部 */
    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(fp);
        return -1;
    }

    /* 步骤2: 逐个升级文件 */
    printf("\n========================================\n");
    printf("  Starting upgrade process\n");
    printf("========================================\n\n");

    for (uint32_t i = 0; i < header.file_count && i < MKKIMG_MAX_FILES; i++) {
        mkkimg_file_info_t *file_info = &header.files[i];
        UPGRADE_CTX_T ctx;
        uint8_t *file_data = NULL;
        FILE *temp_fp = NULL;

        printf("[%u/%u] Upgrading %s (%s)...\n",
               i + 1, header.file_count,
               file_info->filename,
               get_type_name(file_info->file_type));

        /* 映射到升级类型 */
        UPGRADE_TYPE_E upgrade_type = mkkimg_type_to_upgrade_type(file_info->file_type);

        /* 如果类型未知，尝试从文件名推断 */
        if (upgrade_type == UPGRADE_TYPE_NONE) {
            if (strstr(file_info->filename, "loader")) {
                upgrade_type = UPGRADE_TYPE_LOADER;
            } else if (strstr(file_info->filename, "kernel")) {
                upgrade_type = UPGRADE_TYPE_KERNEL;
            } else if (strstr(file_info->filename, "fdt") || strstr(file_info->filename, "dtb")) {
                upgrade_type = UPGRADE_TYPE_FDT;
            } else if (strstr(file_info->filename, "uboot")) {
                upgrade_type = UPGRADE_TYPE_UBOOT;
            } else if (strstr(file_info->filename, "teeos")) {
                upgrade_type = UPGRADE_TYPE_TEEOS;
            }
        }

        if (upgrade_type == UPGRADE_TYPE_NONE) {
            fprintf(stderr, "  Warning: Unknown file type 0x%08X, skipping...\n\n", file_info->file_type);
            continue;
        }

        /* 读取文件数据 */
        file_data = (uint8_t*)malloc(file_info->size);
        if (!file_data) {
            fprintf(stderr, "  Error: Cannot allocate memory\n");
            ret = -1;
            break;
        }

        if (fseek(fp, file_info->offset, SEEK_SET) != 0 ||
            fread(file_data, 1, file_info->size, fp) != file_info->size) {
            fprintf(stderr, "  Error: Cannot read file data\n");
            free(file_data);
            ret = -1;
            break;
        }

        /* 创建临时文件 */
        snprintf(temp_file, sizeof(temp_file), "/tmp/%s", file_info->filename);
        temp_fp = fopen(temp_file, "wb");
        if (!temp_fp) {
            fprintf(stderr, "  Error: Cannot create temp file %s\n", temp_file);
            free(file_data);
            ret = -1;
            break;
        }

        if (fwrite(file_data, 1, file_info->size, temp_fp) != file_info->size) {
            fprintf(stderr, "  Error: Cannot write temp file\n");
            fclose(temp_fp);
            free(file_data);
            ret = -1;
            break;
        }

        fclose(temp_fp);
        free(file_data);

        /* 初始化升级上下文 */
        memset(&ctx, 0, sizeof(ctx));
        ctx.type = upgrade_type;
        strncpy(ctx.image_path, temp_file, sizeof(ctx.image_path) - 1);
        ctx.format = IMAGE_FORMAT_RAW;  /* 签名文件通常是RAW格式 */
        ctx.verify = false;  /* 已经验证过CRC */
        ctx.force = false;

        /* 执行升级 */
        if (upgrade_init(&ctx) < 0) {
            fprintf(stderr, "  Error: Upgrade init failed\n");
            unlink(temp_file);
            ret = -1;
            break;
        }

        /* 执行升级（使用配置驱动流程） */
        char *filename = basename(temp_file);
        if (upgrade_execute_generic(filename, temp_file, NULL) < 0) {
            fprintf(stderr, "  Error: Upgrade execute failed\n");
            unlink(temp_file);
            ret = -1;
            break;
        }
        unlink(temp_file);

        printf("  ✓ %s upgraded successfully\n\n", file_info->filename);
        success_count++;
    }

    fclose(fp);

    /* 显示结果 */
    printf("========================================\n");
    if (ret == 0 && success_count == header.file_count) {
        printf("✓ Package upgrade completed successfully!\n");
        printf("  Total: %u files upgraded\n", success_count);
        printf("\nPlease reboot the system to apply changes.\n");
    } else {
        printf("✗ Package upgrade failed!\n");
        printf("  Success: %u/%u files\n", success_count, header.file_count);
    }
    printf("========================================\n");

    return ret;
}
