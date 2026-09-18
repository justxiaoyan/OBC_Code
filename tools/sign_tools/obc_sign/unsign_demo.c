#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "obc_sign.h"

// 解包函数
int unpack_uboot(int argc, const char *packed_file_path, const char *output_uboot_path)
{
    // 打开打包后的文件
    FILE *packed_file = fopen(packed_file_path, "rb");
    if (!packed_file)
    {
        fprintf(stderr, "Failed to open packed file: %s\n", packed_file_path);
        return -1;
    }

    // 读取结构体（512字节）
    OBC_PACK_HEAD_T header;
    if (fread(&header, sizeof(OBC_PACK_HEAD_T), 1, packed_file) != 1)
    {
        fprintf(stderr, "Failed to read header from packed file\n");
        fclose(packed_file);
        return -1;
    }

    // 验证魔数
    if (strncmp(header.magic, OBC_MAGIC, OBC_MAGIC_LEN - 1) != 0)
    {
        fprintf(stderr, "Invalid magic number: expected '%s', got '%.6s'\n",
                OBC_MAGIC, header.magic);
        fclose(packed_file);
        return -1;
    }

    // 验证文件大小合理性
    if (header.file_size == 0 || header.file_size > 0x10000000)  // 限制最大256MB
    {
        fprintf(stderr, "Invalid file size: %u bytes\n", header.file_size);
        fclose(packed_file);
        return -1;
    }

    // 打印结构体内容
    printf("\n========== OBC Package Header ==========\n");
    printf("Magic:       %.6s\n", header.magic);
    printf("Pack File:   %s\n", header.pack_file);
    printf("File Name:   %s\n", header.file_name);
    printf("File Size:   %u bytes\n", header.file_size);
    printf("CRC16:       0x%04X\n", header.crc16);
    printf("Head Write:  %u (%s)\n", header.head_write_flag, header.head_write_flag ? "YES" : "NO");
    printf("========================================\n\n");

    // 如果只有一个参数，只打印信息不解包
    if (2 == argc)
    {
        fclose(packed_file);
        return 0;
    }

    // 读取文件内容
    uint8_t *uboot_data = malloc(header.file_size);
    if (!uboot_data)
    {
        fprintf(stderr, "Failed to allocate memory for data (%u bytes)\n", header.file_size);
        fclose(packed_file);
        return -1;
    }

    size_t read_bytes = fread(uboot_data, 1, header.file_size, packed_file);
    fclose(packed_file);

    if (read_bytes != header.file_size)
    {
        fprintf(stderr, "Failed to read complete data: read %zu, expected %u\n",
                read_bytes, header.file_size);
        free(uboot_data);
        return -1;
    }

    // 计算并验证CRC16
    uint16_t calculated_crc = calculate_crc16(uboot_data, header.file_size);
    printf("CRC16 Verification:\n");
    printf("  Stored CRC:     0x%04X\n", header.crc16);
    printf("  Calculated CRC: 0x%04X\n", calculated_crc);

    if (calculated_crc != header.crc16)
    {
        fprintf(stderr, "CRC16 verification FAILED! Data may be corrupted.\n");
        free(uboot_data);
        return -1;
    }
    printf("  Status:         PASS\n\n");

    // 将数据保存到文件
    FILE *output_uboot_file = fopen(output_uboot_path, "wb");
    if (!output_uboot_file)
    {
        fprintf(stderr, "Failed to open output file: %s\n", output_uboot_path);
        free(uboot_data);
        return -1;
    }

    if (fwrite(uboot_data, 1, header.file_size, output_uboot_file) != header.file_size)
    {
        fprintf(stderr, "Failed to write data to output file\n");
        fclose(output_uboot_file);
        free(uboot_data);
        return -1;
    }

    fclose(output_uboot_file);
    free(uboot_data);

    printf("Unpacking completed successfully.\n");
    printf("  Output file: %s\n", output_uboot_path);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *output_uboot_path = NULL;
    const char *packed_file_path = NULL;

    if (argc < 2)
    {
        printf("Usage: %s <packed_file_path>\n", argv[0]);
        printf("Usage: %s <packed_file_path> <output_uboot_path>\n", argv[0]);
        return -1;
    }

    packed_file_path = argv[1];
    if (argc != 2)
        output_uboot_path = argv[2];

    return unpack_uboot(argc, packed_file_path, output_uboot_path);
}
