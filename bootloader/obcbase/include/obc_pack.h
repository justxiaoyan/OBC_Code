#ifndef __OBC_PACK_H__
#define __OBC_PACK_H__

#include <linux/types.h>

// 魔数定义
#define OBC_MAGIC "OBCFS"
#define OBC_MAGIC_LEN 6
#define OBC_HEADER_SIZE 512

#pragma pack(push, 1) 
// OBC打包头结构体（使用联合体确保512字节大小）
typedef union {
    struct {
        char magic[OBC_MAGIC_LEN];  // 魔数 "OBCFS"
        u32 file_size;              // 文件大小
        u16 crc16;                  // CRC16校验值
        u16 head_write_flag;        // 头部写入标志：1=需要写入升级分区，0=不需要
        char pack_file[64];         // 输出文件名
        char file_name[64];         // 原始文件名
    } __attribute__((packed));
    u8 raw[OBC_HEADER_SIZE];        // 确保整个结构体为512字节
} OBC_PACK_HEAD_T;
#pragma pack(pop)

/**
 * obc_verify_pack_header - 验证OBC打包头
 * @header: 打包头指针
 * @data: 实际数据指针（512字节头部之后的数据）
 *
 * 返回值: 0=成功, -1=魔数错误, -2=CRC校验失败
 */
int obc_verify_pack_header(OBC_PACK_HEAD_T *header, const u8 *data);

/**
 * obc_calculate_crc16 - 计算CRC16校验值
 * @data: 数据指针
 * @length: 数据长度
 *
 * 返回值: CRC16校验值
 */
u16 obc_calculate_crc16(const u8 *data, u32 length);

#endif /* __OBC_PACK_H__ */
