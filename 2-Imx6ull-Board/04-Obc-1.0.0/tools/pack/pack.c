#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "pack.h"

// 打包函数
int pack_uboot(const char *bin_path, const char *output_path)
{
    // 打开input file文件
    FILE *file = fopen(bin_path, "rb");
    if (!file)
    {
        printf("Failed to open input file %s\n", bin_path);
        return -1;
    }

    // 获取input file文件大小
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 初始化结构体
    OBC_PACK_HEAD_T header = {0};

    // 填充pack_file字段为目标输出文件名（取前8个字符）
    memset(header.pack_file, 0, sizeof(header.pack_file));
    strncpy(header.pack_file, output_path, 31);

    // 填充file_name字段为原始uboot文件名（取文件名部分）
    const char *file_name_start = strrchr(bin_path, '/');
    if (file_name_start == NULL)
        file_name_start = bin_path; // 如果路径中没有'/'，直接使用整个路径
    else
        file_name_start++; // 跳过'/'字符
    strncpy(header.file_name, file_name_start, 31);
    header.file_name[31] = '\0'; // 确保字符串以null结尾

    header.file_size = size;

    // 读取input file内容到缓冲区
    uint8_t *data = malloc(size);
    if (!data)
    {
        perror("Failed to allocate memory for uboot data");
        fclose(file);
        return -1;
    }
    fread(data, 1, size, file);
    fclose(file);

    // 打开输出文件
    FILE *output_file = fopen(output_path, "wb");
    if (!output_file)
    {
        perror("Failed to open output file");
        free(data);
        return -1;
    }

    // 写入结构体
    fwrite(&header, sizeof(OBC_PACK_HEAD_T), 1, output_file);

    // 写入input file内容
    fwrite(data, 1, size, output_file);

    // 关闭文件
    fclose(output_file);
    free(data);

    printf("Packing completed successfully. Output file: %s\n", output_path);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <bin_path> <output_path>\n", argv[0]);
        return -1;
    }

    const char *bin_path = argv[1];
    const char *output_path = argv[2];

    return pack_uboot(bin_path, output_path);
}