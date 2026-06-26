/*
 * EMS Package Header Definition
 * 升级包签名头定义（参考 ems_pack.h）
 */

#ifndef __EMS_PKG_HEADER_H__
#define __EMS_PKG_HEADER_H__

#include <stdint.h>

/* 魔数定义（从 ems_pack.h） */
#define EMS_MAGIC           "EMSFS"
#define EMS_MAGIC_LEN       6
#define EMS_HEADER_SIZE     512

/* 升级包头结构体（与 ems_pack.h 完全一致） */
typedef union {
    struct {
        char magic[EMS_MAGIC_LEN];      /* 魔数 "EMSFS" */
        uint32_t file_size;             /* 文件大小 */
        uint16_t crc16;                 /* CRC16 校验值 */
        uint16_t head_write_flag;       /* 头部写入标志 */
        char pack_file[64];             /* 输出文件名 */
        char file_name[64];             /* 原始文件名 */
    } __attribute__((packed));
    uint8_t raw[EMS_HEADER_SIZE];       /* 确保整个结构体为 512 字节 */
} ems_pkg_header_t;

/* 包头操作接口 */
int ems_pkg_header_parse(const char *file, ems_pkg_header_t *header);
int ems_pkg_header_verify(const ems_pkg_header_t *header);
int ems_pkg_get_data_offset(const char *file, uint32_t *offset);
int ems_pkg_get_data_size(const char *file, uint32_t *size);
int ems_pkg_has_header(const char *file);
void ems_pkg_print_header(const ems_pkg_header_t *header);

/* CRC16 计算（用于验证） */
uint16_t ems_calculate_crc16(const uint8_t *data, uint32_t length);

#endif /* __EMS_PKG_HEADER_H__ */
