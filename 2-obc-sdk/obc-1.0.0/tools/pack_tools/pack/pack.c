#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <getopt.h>
#include "pack.h"

// 打包函数
int pack_func(const char *bin_path, const char *output_path, uint16_t head_write_flag)
{
    // 打开input file文件
    FILE *file = fopen(bin_path, "rb");
    if (!file)
    {
        fprintf(stderr, "Failed to open input file: %s\n", bin_path);
        return -1;
    }

    // 获取input file文件大小
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Failed to seek file\n");
        fclose(file);
        return -1;
    }

    long size_long = ftell(file);
    if (size_long < 0 || size_long > UINT32_MAX)
    {
        fprintf(stderr, "Invalid file size: %ld\n", size_long);
        fclose(file);
        return -1;
    }

    uint32_t size = (uint32_t)size_long;
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Failed to seek to file start\n");
        fclose(file);
        return -1;
    }

    // 初始化结构体（使用联合体确保512字节）
    OBC_PACK_HEAD_T header;
    memset(&header, 0, sizeof(OBC_PACK_HEAD_T));

    // 设置魔数
    strncpy(header.magic, OBC_MAGIC, OBC_MAGIC_LEN - 1);
    header.magic[OBC_MAGIC_LEN - 1] = '\0';

    // 填充pack_file字段为目标输出文件名（取文件名部分）
    const char *pack_file_name = strrchr(output_path, '/');
    if (pack_file_name == NULL)
        pack_file_name = output_path;
    else
        pack_file_name++;

    strncpy(header.pack_file, pack_file_name, sizeof(header.pack_file) - 1);
    header.pack_file[sizeof(header.pack_file) - 1] = '\0';

    // 填充file_name字段为原始文件名（取文件名部分）
    const char *file_name_start = strrchr(bin_path, '/');
    if (file_name_start == NULL)
        file_name_start = bin_path;
    else
        file_name_start++;

    strncpy(header.file_name, file_name_start, sizeof(header.file_name) - 1);
    header.file_name[sizeof(header.file_name) - 1] = '\0';

    header.file_size = size;
    header.head_write_flag = head_write_flag;

    // 读取input file内容到缓冲区
    uint8_t *data = malloc(size);
    if (!data)
    {
        fprintf(stderr, "Failed to allocate memory for data (%u bytes)\n", size);
        fclose(file);
        return -1;
    }

    size_t read_bytes = fread(data, 1, size, file);
    fclose(file);

    if (read_bytes != size)
    {
        fprintf(stderr, "Failed to read complete file: read %zu, expected %u\n", read_bytes, size);
        free(data);
        return -1;
    }

    // 计算CRC16
    header.crc16 = calculate_crc16(data, size);

    // 打开输出文件
    FILE *output_file = fopen(output_path, "wb");
    if (!output_file)
    {
        fprintf(stderr, "Failed to open output file: %s\n", output_path);
        free(data);
        return -1;
    }

    // 写入结构体（整个512字节）
    if (fwrite(&header, sizeof(OBC_PACK_HEAD_T), 1, output_file) != 1)
    {
        fprintf(stderr, "Failed to write header\n");
        fclose(output_file);
        free(data);
        return -1;
    }

    // 写入input file内容
    if (fwrite(data, 1, size, output_file) != size)
    {
        fprintf(stderr, "Failed to write data\n");
        fclose(output_file);
        free(data);
        return -1;
    }

    // 关闭文件
    fclose(output_file);
    free(data);

    printf("Packing completed successfully. Output file: %s\n", output_path);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *bin_path = NULL;
    const char *output_path = NULL;
    uint16_t head_write_flag = 0;
    int opt;

    // 解析命令行参数
    while ((opt = getopt(argc, argv, "h")) != -1)
    {
        switch (opt)
        {
        case 'h':
            head_write_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-h] <bin_path> <output_path>\n", argv[0]);
            fprintf(stderr, "  -h: Set head_write_flag to 1 (write header to upgrade partition)\n");
            return -1;
        }
    }

    // 获取剩余的位置参数
    if (optind + 2 != argc)
    {
        fprintf(stderr, "Usage: %s [-h] <bin_path> <output_path>\n", argv[0]);
        fprintf(stderr, "  -h: Set head_write_flag to 1 (write header to upgrade partition)\n");
        return -1;
    }

    bin_path = argv[optind];
    output_path = argv[optind + 1];

    return pack_func(bin_path, output_path, head_write_flag);
}
