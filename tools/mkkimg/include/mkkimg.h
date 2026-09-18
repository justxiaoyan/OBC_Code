/**
 * @file mkkimg.h
 * @brief mkkimg工具头文件 - 用于打包签名后的升级文件
 * @author hushanyan
 * @date 2026-06-17
 * @version 1.0
 */

#ifndef __MKKIMG_H__
#define __MKKIMG_H__

#include <stdint.h>

/* 魔术数 "KIMG" */
#define MKKIMG_MAGIC            0x474D494B

/* 信息头大小 */
#define MKKIMG_HEADER_SIZE      512

/* 文件名最大长度 */
#define MKKIMG_FILENAME_LEN     64

/* 最大文件数量 */
#define MKKIMG_MAX_FILES        6

/* 升级文件类型 (from cmd_updatex.h - obc board config) */
#define UPDATEX_FILE_TYPE_NONE      0
#define UPDATEX_FILE_TYPE_LOADER    1
#define UPDATEX_FILE_TYPE_ATF       2
#define UPDATEX_FILE_TYPE_TEEOS     3
#define UPDATEX_FILE_TYPE_FDT       4
#define UPDATEX_FILE_TYPE_UBOOT     5
#define UPDATEX_FILE_TYPE_KERNEL    6
#define UPDATEX_FILE_TYPE_ROOTFS    7
#define UPDATEX_FILE_TYPE_APPFS     8

/**
 * 文件信息结构 - 80字节
 */
typedef struct {
    char     filename[MKKIMG_FILENAME_LEN]; /* 文件名（64字节） */
    uint32_t file_type;                      /* 文件类型（根据文件名自动识别） */
    uint32_t offset;                         /* 文件在整包中的偏移 */
    uint32_t size;                           /* 文件大小（字节） */
    uint32_t crc32;                          /* 文件CRC32校验值 */
} __attribute__((packed)) mkkimg_file_info_t;

/**
 * 镜像头部结构 - 512字节
 */
typedef struct {
    uint32_t magic;                          /* 魔术数: 0x474D494B "KIMG" */
    uint32_t header_version;                 /* 头部版本号: 0x00010000 (v1.0) */
    uint32_t file_count;                     /* 打包的文件数量 */
    uint32_t total_size;                     /* 整个镜像文件大小（字节） */
    uint32_t data_offset;                    /* 数据区起始偏移（头部之后） */
    uint32_t package_crc16;                  /* 整包CRC16校验值（不包括此字段） */
    uint32_t create_timestamp;               /* 创建时间戳（Unix时间） */
    uint32_t reserved1;                      /* 保留字段 */

    /* 文件信息数组 - 每个80字节，最多6个文件 */
    mkkimg_file_info_t files[6];             /* 6 * 80 = 480字节 */

    /* 头部总大小: 32 + 480 = 512字节 */
} __attribute__((packed)) mkkimg_header_t;

/* 确保头部大小正确 */
_Static_assert(sizeof(mkkimg_header_t) <= MKKIMG_HEADER_SIZE,
               "mkkimg_header_t size exceeds MKKIMG_HEADER_SIZE");

/* 函数声明 */
uint16_t crc16_ccitt(const uint8_t *data, size_t length);
uint32_t crc32(const uint8_t *data, size_t length);
uint32_t get_file_type(const char *filename, const char *platform);

#endif /* __MKKIMG_H__ */
