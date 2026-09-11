/**
 * @file mkkimg.c
 * @brief mkkimg工具主程序 - 打包签名后的升级文件
 * @author hushanyan
 * @date 2026-06-17
 * @version 1.0
 *
 * 用法: mkkimg <pack_dirname> <output_file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>
#include "mkkimg.h"

#define MAX_PATH_LEN    1024
#define READ_BUFFER_SIZE (64 * 1024)  /* 64KB读取缓冲 */

/**
 * 根据文件名识别文件类型（使用固定文件名）
 */
uint32_t get_file_type(const char *filename)
{
    if (strcmp(filename, LOADER_FILE_NAME) == 0) return UPDATEX_FILE_TYPE_LOADER;  /* 1 */
    if (strcmp(filename, ATF_FILE_NAME) == 0)    return UPDATEX_FILE_TYPE_ATF;     /* 2 */
    if (strcmp(filename, TEEOS_FILE_NAME) == 0)  return UPDATEX_FILE_TYPE_TEEOS;   /* 3 */
    if (strcmp(filename, FDT_FILE_NAME) == 0)    return UPDATEX_FILE_TYPE_FDT;     /* 4 */
    if (strcmp(filename, UBOOT_FILE_NAME) == 0)  return UPDATEX_FILE_TYPE_UBOOT;   /* 5 */
    if (strcmp(filename, KERNEL_FILE_NAME) == 0) return UPDATEX_FILE_TYPE_KERNEL;  /* 6 */
    if (strcmp(filename, ROOTFS_FILE_NAME) == 0) return UPDATEX_FILE_TYPE_ROOTFS;  /* 7 */
    if (strcmp(filename, APPFS_FILE_NAME) == 0)  return UPDATEX_FILE_TYPE_APPFS;   /* 8 */

    return UPDATEX_FILE_TYPE_NONE;
}

/**
 * 获取文件大小
 */
static off_t get_file_size(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) < 0) {
        return -1;
    }
    return st.st_size;
}

/**
 * 计算文件的CRC32
 */
static uint32_t calculate_file_crc32(const char *filepath)
{
    FILE *fp;
    uint8_t buffer[READ_BUFFER_SIZE];
    size_t bytes_read;
    uint32_t crc = 0xFFFFFFFF;
    size_t i, j;

    fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open file '%s' for CRC: %s\n",
                filepath, strerror(errno));
        return 0;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        for (i = 0; i < bytes_read; i++) {
            crc ^= buffer[i];
            for (j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320;
                } else {
                    crc = crc >> 1;
                }
            }
        }
    }

    fclose(fp);
    return ~crc;
}

/**
 * 扫描目录，收集所有-sign.bin文件
 */
static int scan_directory(const char *dirname, char files[][MAX_PATH_LEN], int *file_count)
{
    DIR *dir;
    struct dirent *entry;
    char filepath[MAX_PATH_LEN];
    struct stat st;

    *file_count = 0;

    dir = opendir(dirname);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory '%s': %s\n",
                dirname, strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        /* 跳过.和.. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* 只处理-sign.bin结尾的文件 */
        if (strstr(entry->d_name, "-sign.bin") == NULL) {
            continue;
        }

        snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);

        /* 检查是否为普通文件 */
        if (stat(filepath, &st) < 0) {
            fprintf(stderr, "Warning: Cannot stat '%s': %s\n",
                    filepath, strerror(errno));
            continue;
        }

        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        /* 检查文件数量限制 */
        if (*file_count >= MKKIMG_MAX_FILES) {
            fprintf(stderr, "Error: Too many files (max %d)\n", MKKIMG_MAX_FILES);
            closedir(dir);
            return -1;
        }

        /* 添加到文件列表 */
        strncpy(files[*file_count], filepath, MAX_PATH_LEN - 1);
        files[*file_count][MAX_PATH_LEN - 1] = '\0';
        (*file_count)++;
    }

    closedir(dir);

    if (*file_count == 0) {
        fprintf(stderr, "Error: No *-sign.bin files found in '%s'\n", dirname);
        return -1;
    }

    return 0;
}

/**
 * 打印使用帮助
 */
static void print_usage(const char *prog)
{
    printf("Usage: %s <pack_dirname> <output_file>\n", prog);
    printf("\n");
    printf("Description:\n");
    printf("  Pack signed upgrade files (*-sign.bin) into a single image.\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  pack_dirname  - Directory containing *-sign.bin files\n");
    printf("  output_file   - Output image file path\n");
    printf("\n");
    printf("Example:\n");
    printf("  %s ./signed_files upgrade.img\n", prog);
    printf("\n");
}

/**
 * 打印头部信息
 */
static void print_header_info(const mkkimg_header_t *header)
{
    int i;
    const char *type_names[] = {
        "None", "Loader", "ATF", "TEE-OS", "FDT", "U-Boot", "Kernel", "RootFS", "AppFS"
    };

    printf("\n========== Image Header Info ==========\n");
    printf("Magic:           0x%08X (KIMG)\n", header->magic);
    printf("Version:         0x%08X\n", header->header_version);
    printf("File Count:      %u\n", header->file_count);
    printf("Total Size:      %u bytes (%.2f MB)\n",
           header->total_size, header->total_size / (1024.0 * 1024.0));
    printf("Data Offset:     %u bytes\n", header->data_offset);
    printf("Package CRC16:   0x%04X\n", header->package_crc16);
    printf("Create Time:     %u (%s)\n", header->create_timestamp,
           ctime((time_t*)&header->create_timestamp));

    printf("\n========== Files Info ==========\n");
    printf("%-3s %-32s %-8s %-10s %-10s %-10s\n",
           "No.", "Filename", "Type", "Offset", "Size", "CRC32");
    printf("-----------------------------------------------------------------------------------\n");

    for (i = 0; i < header->file_count; i++) {
        const char *type_name = (header->files[i].file_type <= 8) ?
                                 type_names[header->files[i].file_type] : "Unknown";

        printf("%-3d %-32s %-8s 0x%08X %-10u 0x%08X\n",
               i + 1,
               header->files[i].filename,
               type_name,
               header->files[i].offset,
               header->files[i].size,
               header->files[i].crc32);
    }
    printf("\n");
}

/**
 * 主函数
 */
int main(int argc, char *argv[])
{
    char files[MKKIMG_MAX_FILES][MAX_PATH_LEN];
    int file_count = 0;
    mkkimg_header_t header;
    FILE *fp_out = NULL;
    FILE *fp_in = NULL;
    uint8_t *buffer = NULL;
    uint32_t current_offset;
    int i;
    size_t bytes_read;
    int ret = 0;

    /* 检查参数 */
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *pack_dir = argv[1];
    const char *output_file = argv[2];

    printf("mkkimg v1.0 - Upgrade Image Packer\n");
    printf("===================================\n");
    printf("Input directory: %s\n", pack_dir);
    printf("Output file:     %s\n\n", output_file);

    /* 扫描目录 */
    printf("Scanning directory...\n");
    if (scan_directory(pack_dir, files, &file_count) < 0) {
        return 1;
    }
    printf("Found %d signed file(s)\n\n", file_count);

    /* 初始化头部 */
    memset(&header, 0, sizeof(header));
    header.magic = MKKIMG_MAGIC;
    header.header_version = 0x00010000;  /* v1.0 */
    header.file_count = file_count;
    header.data_offset = MKKIMG_HEADER_SIZE;
    header.create_timestamp = (uint32_t)time(NULL);

    /* 收集文件信息并计算偏移 */
    current_offset = header.data_offset;
    for (i = 0; i < file_count; i++) {
        off_t file_size = get_file_size(files[i]);
        if (file_size < 0) {
            fprintf(stderr, "Error: Cannot get size of '%s'\n", files[i]);
            return 1;
        }

        /* 提取文件名（basename） */
        char *base = basename(files[i]);
        strncpy(header.files[i].filename, base, MKKIMG_FILENAME_LEN - 1);
        header.files[i].filename[MKKIMG_FILENAME_LEN - 1] = '\0';

        /* 识别文件类型 */
        header.files[i].file_type = get_file_type(base);

        /* 设置偏移和大小 */
        header.files[i].offset = current_offset;
        header.files[i].size = (uint32_t)file_size;

        /* 计算文件CRC32 */
        printf("Calculating CRC32 for %s...\n", base);
        header.files[i].crc32 = calculate_file_crc32(files[i]);

        /* 更新偏移 */
        current_offset += file_size;
    }

    header.total_size = current_offset;

    /* 计算整包CRC16（不包括package_crc16字段本身） */
    printf("\nCalculating package CRC16...\n");
    uint16_t pkg_crc = crc16_ccitt((uint8_t*)&header,
                                    offsetof(mkkimg_header_t, package_crc16));
    header.package_crc16 = pkg_crc;

    /* 打印头部信息 */
    print_header_info(&header);

    /* 创建输出文件 */
    printf("Creating output image...\n");
    fp_out = fopen(output_file, "wb");
    if (!fp_out) {
        fprintf(stderr, "Error: Cannot create output file '%s': %s\n",
                output_file, strerror(errno));
        return 1;
    }

    /* 写入头部 */
    if (fwrite(&header, 1, MKKIMG_HEADER_SIZE, fp_out) != MKKIMG_HEADER_SIZE) {
        fprintf(stderr, "Error: Failed to write header\n");
        fclose(fp_out);
        unlink(output_file);
        return 1;
    }

    /* 分配读取缓冲 */
    buffer = (uint8_t*)malloc(READ_BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "Error: Cannot allocate buffer\n");
        fclose(fp_out);
        unlink(output_file);
        return 1;
    }

    /* 依次写入文件数据 */
    for (i = 0; i < file_count; i++) {
        printf("Packing [%d/%d]: %s (%u bytes)...\n",
               i + 1, file_count, header.files[i].filename, header.files[i].size);

        fp_in = fopen(files[i], "rb");
        if (!fp_in) {
            fprintf(stderr, "Error: Cannot open '%s': %s\n",
                    files[i], strerror(errno));
            ret = 1;
            goto cleanup;
        }

        /* 复制文件内容 */
        while ((bytes_read = fread(buffer, 1, READ_BUFFER_SIZE, fp_in)) > 0) {
            if (fwrite(buffer, 1, bytes_read, fp_out) != bytes_read) {
                fprintf(stderr, "Error: Failed to write data from '%s'\n", files[i]);
                ret = 1;
                fclose(fp_in);
                goto cleanup;
            }
        }

        fclose(fp_in);
        fp_in = NULL;
    }

    printf("\n✓ Image created successfully: %s\n", output_file);
    printf("  Total size: %u bytes (%.2f MB)\n",
           header.total_size, header.total_size / (1024.0 * 1024.0));

cleanup:
    if (buffer) free(buffer);
    if (fp_in) fclose(fp_in);
    if (fp_out) fclose(fp_out);

    if (ret != 0) {
        unlink(output_file);
    }

    return ret;
}
