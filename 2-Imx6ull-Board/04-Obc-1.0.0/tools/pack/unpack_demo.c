#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "pack.h"



// 解包函数
int unpack_uboot(const char *packed_file_path, const char *output_uboot_path)
{
    // 打开打包后的文件
    FILE *packed_file = fopen(packed_file_path, "rb");
    if (!packed_file)
    {
        perror("Failed to open packed file");
        return -1;
    }

    // 读取结构体
    OBC_PACK_HEAD_T header;
    if (fread(&header, sizeof(OBC_PACK_HEAD_T), 1, packed_file) != 1)
    {
        perror("Failed to read header from packed file");
        fclose(packed_file);
        return -1;
    }

    // 打印结构体内容
    printf("Header Information:\n");
    printf("Flag: %s\n", header.pack_file);
    printf("File Name: %s\n", header.file_name);
    printf("File Size: %d bytes\n", header.file_size);
    printf("CRC: 0x%X\n", header.crc);

    // 读取uboot.bin内容
    uint8_t *uboot_data = malloc(header.file_size);
    if (!uboot_data)
    {
        perror("Failed to allocate memory for uboot data");
        fclose(packed_file);
        return -1;
    }
    if (fread(uboot_data, 1, header.file_size, packed_file) != (size_t)header.file_size)
    {
        perror("Failed to read uboot data from packed file");
        free(uboot_data);
        fclose(packed_file);
        return -1;
    }
    fclose(packed_file);

    // 将uboot数据保存到文件
    FILE *output_uboot_file = fopen(output_uboot_path, "wb");
    if (!output_uboot_file)
    {
        perror("Failed to open output uboot file");
        free(uboot_data);
        return -1;
    }
    fwrite(uboot_data, 1, header.file_size, output_uboot_file);
    fclose(output_uboot_file);

    free(uboot_data);

    printf("Unpacking completed successfully. Uboot file saved to: %s\n", output_uboot_path);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <packed_file_path> <output_uboot_path>\n", argv[0]);
        return -1;
    }

    const char *packed_file_path = argv[1];
    const char *output_uboot_path = argv[2];

    return unpack_uboot(packed_file_path, output_uboot_path);
}