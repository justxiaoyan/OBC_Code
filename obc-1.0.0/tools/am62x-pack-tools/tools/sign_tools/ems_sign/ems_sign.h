

#ifndef __PACK_H__
#define __PACK_H__

#include <stdint.h>

// 魔数定义
#define EMS_MAGIC "EMSFS"
#define EMS_MAGIC_LEN 6
#define EMS_HEADER_SIZE 512

// EMS打包头结构体（使用联合体确保512字节大小）
typedef union {
    struct {
        char magic[EMS_MAGIC_LEN];  // 魔数 "EMSFS"
        uint32_t file_size;         // 文件大小
        uint16_t crc16;             // CRC16校验值
        uint16_t head_write_flag;   // 头部写入标志：1=需要写入升级分区，0=不需要
        char pack_file[64];         // 输出文件名
        char file_name[64];         // 原始文件名
    } __attribute__((packed));
    uint8_t raw[EMS_HEADER_SIZE];   // 确保整个结构体为512字节
} EMS_PACK_HEAD_T;

// CRC16计算函数声明
uint16_t calculate_crc16(const uint8_t *data, uint32_t length);

#endif


