/*
 * umkkimg - Unpack and analyze a platform factory image
 * 解析平台 factory 镜像文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

/* 包含mkkimg头文件定义 */
#include "mkkimg.h"

/* CRC16查表法 */
extern uint16_t crc16_ccitt(const uint8_t *data, size_t length);
extern uint32_t crc32(const uint8_t *data, size_t length);

/**
 * 获取类型名称
 */
static const char *get_type_name(uint32_t type)
{
    const char *type_names[] = {
        "None", "Loader", "ATF", "TEE-OS", "FDT", "U-Boot", "Kernel", "RootFS", "AppFS"
    };
    return (type <= 8) ? type_names[type] : "Unknown";
}

/**
 * 打印头部信息
 */
static void print_header(const mkkimg_header_t *header)
{
    printf("\n========================================\n");
    printf("  Factory Image Header\n");
    printf("========================================\n\n");

    printf("Magic:           0x%08X", header->magic);
    if (header->magic == MKKIMG_MAGIC) {
        printf(" (KIMG) ✓\n");
    } else {
        printf(" (INVALID) ✗\n");
    }

    printf("Version:         0x%08X\n", header->header_version);
    printf("File Count:      %u\n", header->file_count);
    printf("Total Size:      %u bytes (%.2f MB)\n",
           header->total_size, header->total_size / (1024.0 * 1024.0));
    printf("Data Offset:     %u bytes (0x%X)\n", header->data_offset, header->data_offset);
    printf("Package CRC16:   0x%04X\n", header->package_crc16);

    time_t timestamp = (time_t)header->create_timestamp;
    printf("Create Time:     %u (%s", header->create_timestamp, ctime(&timestamp));
    printf("\n");
}

/**
 * 打印文件列表
 */
static void print_files(const mkkimg_header_t *header)
{
    printf("========================================\n");
    printf("  File List\n");
    printf("========================================\n\n");

    printf("%-3s %-32s %-10s %-10s %-12s %-10s\n",
           "No.", "Filename", "Type", "Type#", "Offset", "Size");
    printf("-------------------------------------------------------------------------------------\n");

    for (uint32_t i = 0; i < header->file_count && i < MKKIMG_MAX_FILES; i++) {
        printf("%-3u %-32s %-10s %-10u 0x%08X   %-10u\n",
               i + 1,
               header->files[i].filename,
               get_type_name(header->files[i].file_type),
               header->files[i].file_type,
               header->files[i].offset,
               header->files[i].size);
    }
    printf("\n");
}

/**
 * 验证镜像
 */
static int verify_image(const char *image_path, const mkkimg_header_t *header)
{
    FILE *fp;
    uint16_t calc_crc16;
    int ret = 0;

    printf("========================================\n");
    printf("  Verification\n");
    printf("========================================\n\n");

    /* 验证CRC16 */
    calc_crc16 = crc16_ccitt((uint8_t*)header, offsetof(mkkimg_header_t, package_crc16));
    printf("CRC16 Check:     ");
    if (calc_crc16 == header->package_crc16) {
        printf("PASS ✓ (0x%04X)\n", header->package_crc16);
    } else {
        printf("FAIL ✗ (Expected: 0x%04X, Got: 0x%04X)\n",
               header->package_crc16, calc_crc16);
        ret = -1;
    }

    /* 验证每个文件的CRC32 */
    fp = fopen(image_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open image file\n");
        return -1;
    }

    printf("\nFile CRC32 Check:\n");
    for (uint32_t i = 0; i < header->file_count && i < MKKIMG_MAX_FILES; i++) {
        uint8_t *data = malloc(header->files[i].size);
        if (!data) {
            fprintf(stderr, "  [%u] %s: Memory allocation failed\n",
                    i + 1, header->files[i].filename);
            ret = -1;
            continue;
        }

        if (fseek(fp, header->files[i].offset, SEEK_SET) != 0 ||
            fread(data, 1, header->files[i].size, fp) != header->files[i].size) {
            fprintf(stderr, "  [%u] %s: Read failed\n",
                    i + 1, header->files[i].filename);
            free(data);
            ret = -1;
            continue;
        }

        uint32_t calc_crc32 = crc32(data, header->files[i].size);
        printf("  [%u] %-32s ", i + 1, header->files[i].filename);
        if (calc_crc32 == header->files[i].crc32) {
            printf("PASS ✓ (0x%08X)\n", header->files[i].crc32);
        } else {
            printf("FAIL ✗ (Expected: 0x%08X, Got: 0x%08X)\n",
                   header->files[i].crc32, calc_crc32);
            ret = -1;
        }

        free(data);
    }

    fclose(fp);

    printf("\n");
    if (ret == 0) {
        printf("✓ All checks PASSED\n");
    } else {
        printf("✗ Some checks FAILED\n");
    }
    printf("\n");

    return ret;
}

/**
 * 提取文件
 */
static int extract_files(const char *image_path, const mkkimg_header_t *header, const char *output_dir)
{
    FILE *fp_in, *fp_out;
    char output_path[512];
    uint8_t *buffer;
    int ret = 0;

    printf("========================================\n");
    printf("  Extracting Files\n");
    printf("========================================\n\n");

    fp_in = fopen(image_path, "rb");
    if (!fp_in) {
        fprintf(stderr, "Error: Cannot open image file\n");
        return -1;
    }

    for (uint32_t i = 0; i < header->file_count && i < MKKIMG_MAX_FILES; i++) {
        snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, header->files[i].filename);

        printf("[%u/%u] Extracting %s... ", i + 1, header->file_count, header->files[i].filename);
        fflush(stdout);

        buffer = malloc(header->files[i].size);
        if (!buffer) {
            printf("FAIL (memory)\n");
            ret = -1;
            continue;
        }

        if (fseek(fp_in, header->files[i].offset, SEEK_SET) != 0 ||
            fread(buffer, 1, header->files[i].size, fp_in) != header->files[i].size) {
            printf("FAIL (read)\n");
            free(buffer);
            ret = -1;
            continue;
        }

        fp_out = fopen(output_path, "wb");
        if (!fp_out) {
            printf("FAIL (create)\n");
            free(buffer);
            ret = -1;
            continue;
        }

        if (fwrite(buffer, 1, header->files[i].size, fp_out) != header->files[i].size) {
            printf("FAIL (write)\n");
            fclose(fp_out);
            free(buffer);
            ret = -1;
            continue;
        }

        fclose(fp_out);
        free(buffer);

        printf("OK (%u bytes)\n", header->files[i].size);
    }

    fclose(fp_in);

    printf("\n");
    if (ret == 0) {
        printf("✓ All files extracted to: %s\n", output_dir);
    } else {
        printf("✗ Some files failed to extract\n");
    }
    printf("\n");

    return ret;
}

/**
 * 打印使用帮助
 */
static void print_usage(const char *prog)
{
    printf("Usage: %s <platform-factory.bin> [options]\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  -i, --info       Show image information (default)\n");
    printf("  -v, --verify     Verify image integrity (CRC16/CRC32)\n");
    printf("  -x, --extract    Extract files to output directory\n");
    printf("  -o, --output     Output directory for extraction (default: ./extracted)\n");
    printf("  -h, --help       Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s am62x-factory.bin                    # Show info\n", prog);
    printf("  %s am62x-factory.bin -v                 # Verify integrity\n", prog);
    printf("  %s am62x-factory.bin -x                 # Extract to ./extracted\n", prog);
    printf("  %s am62x-factory.bin -x -o /tmp/out    # Extract to /tmp/out\n", prog);
    printf("\n");
}

/**
 * 主函数
 */
int main(int argc, char *argv[])
{
    const char *image_path = NULL;
    const char *output_dir = "./extracted";
    bool show_info = true;
    bool do_verify = false;
    bool do_extract = false;
    FILE *fp;
    mkkimg_header_t header;
    int ret = 0;

    /* 解析参数 */
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    image_path = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--info") == 0) {
            show_info = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verify") == 0) {
            do_verify = true;
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--extract") == 0) {
            do_extract = true;
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                output_dir = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
        }
    }

    /* 打开镜像文件 */
    fp = fopen(image_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open '%s': %s\n", image_path, strerror(errno));
        return 1;
    }

    /* 读取头部 */
    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);

    /* 验证魔术数 */
    if (header.magic != MKKIMG_MAGIC) {
        fprintf(stderr, "Error: Invalid magic number 0x%08X (expected 0x%08X)\n",
                header.magic, MKKIMG_MAGIC);
        return 1;
    }

    printf("\numkkimg - Factory Image Analyzer\n");
    printf("Image: %s\n", image_path);

    /* 显示信息 */
    if (show_info) {
        print_header(&header);
        print_files(&header);
    }

    /* 验证 */
    if (do_verify) {
        ret = verify_image(image_path, &header);
        if (ret != 0) {
            return 1;
        }
    }

    /* 提取 */
    if (do_extract) {
        /* 创建输出目录 */
        mkdir(output_dir, 0755);

        ret = extract_files(image_path, &header, output_dir);
        if (ret != 0) {
            return 1;
        }
    }

    return 0;
}
