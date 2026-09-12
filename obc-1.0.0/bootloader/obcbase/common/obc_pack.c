#include <obc_pack.h>
#include <linux/crc16.h>
#include <string.h>
#include <stdio.h>

/**
 * obc_calculate_crc16 - 计算CRC16校验值
 * @data: 数据指针
 * @length: 数据长度
 *
 * 使用uboot lib/crc16.c的实现（多项式0x8005，初始值0xFFFF）
 *
 * 返回值: CRC16校验值
 */
u16 obc_calculate_crc16(const u8 *data, u32 length)
{
    return crc16(0xFFFF, data, length);
}

/**
 * obc_verify_pack_header - 验证OBC打包头
 * @header: 打包头指针
 * @data: 实际数据指针（512字节头部之后的数据）
 *
 * 验证步骤：
 * 1. 检查魔数是否为"OBCFS"
 * 2. 计算数据的CRC16并与头部中的CRC16比较
 *
 * 返回值: 0=成功, -1=魔数错误, -2=CRC校验失败
 */
int obc_verify_pack_header(OBC_PACK_HEAD_T *header, const u8 *data)
{
    u16 calculated_crc;

    /* 检查魔数 */
    if (strncmp(header->magic, OBC_MAGIC, OBC_MAGIC_LEN - 1) != 0) {
        printf("OBC Pack: Invalid magic number, expected '%s', got '%.5s'\n",
               OBC_MAGIC, header->magic);
        return -1;
    }

    /* 计算并验证CRC16 */
    calculated_crc = obc_calculate_crc16(data, header->file_size);
    if (calculated_crc != header->crc16) {
        printf("OBC Pack: CRC16 verification failed!\n");
        printf("  Stored CRC:     0x%04X\n", header->crc16);
        printf("  Calculated CRC: 0x%04X\n", calculated_crc);
        printf("  File size:      %u bytes\n", header->file_size);
        return -2;
    }

#if 0
    /* 验证成功，打印信息 */
    printf("OBC Pack: Header verification passed\n");
    printf("  Magic:          %s\n", OBC_MAGIC);
    printf("  File name:      %s\n", header->file_name);
    printf("  File size:      %u bytes\n", header->file_size);
    printf("  CRC16:          0x%04X\n", header->crc16);
    printf("  Head write:     %u (%s)\n", header->head_write_flag,
           header->head_write_flag ? "YES" : "NO");
#endif
    return 0;
}
