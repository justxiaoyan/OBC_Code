

#ifndef __PACK_H__
#define __PACK_H__

#include <stdint.h>

// 魔数定义
#define OBC_MAGIC "OBCFS"
#define OBC_MAGIC_LEN 6
#define OBC_HEADER_SIZE 512

// OBC打包头结构体（使用联合体确保512字节大小）
typedef union {
    struct {
        char magic[OBC_MAGIC_LEN];  // 魔数 "OBCFS"
        uint32_t file_size;         // 文件大小
        uint16_t crc16;             // CRC16校验值
        uint16_t head_write_flag;   // 头部写入标志：1=需要写入升级分区，0=不需要
        char pack_file[64];         // 输出文件名
        char file_name[64];         // 原始文件名
    } __attribute__((packed));
    uint8_t raw[OBC_HEADER_SIZE];   // 确保整个结构体为512字节
} OBC_PACK_HEAD_T;

// CRC16计算函数声明
uint16_t calculate_crc16(const uint8_t *data, uint32_t length);

#endif

