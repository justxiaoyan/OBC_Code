/*
 * mkkimg Package Parser and Upgrader
 * 解析mkkimg打包的factory.bin并执行整包升级
 */

#ifndef __MKKIMG_UPGRADE_H__
#define __MKKIMG_UPGRADE_H__

#include <stdint.h>
#include <stdbool.h>

/* mkkimg魔术数 */
#define MKKIMG_MAGIC            0x474D494B  /* "KIMG" */

/* 文件名最大长度 */
#define MKKIMG_FILENAME_LEN     64

/* 最大文件数量 */
#define MKKIMG_MAX_FILES        6

/* 文件名定义（固定） */
#define LOADER_FILE_NAME        "loader-sign.bin"
#define ATF_FILE_NAME           "atf-sign.bin"
#define TEEOS_FILE_NAME         "teeos-sign.bin"
#define FDT_FILE_NAME           "fdt-sign.bin"
#define UBOOT_FILE_NAME         "uboot-sign.bin"
#define KERNEL_FILE_NAME        "kernel-sign.bin"
#define ROOTFS_FILE_NAME        "rootfs-sign.bin"
#define APPFS_FILE_NAME         "appfs-sign.bin"
#define FACTORY_FILE_NAME       "factory.bin"

/* 升级文件类型 (from cmd_updatex.h - ems_board_config.h) */
typedef enum {
    UPDATEX_FILE_TYPE_NONE      = 0,
    UPDATEX_FILE_TYPE_LOADER    = 1,
    UPDATEX_FILE_TYPE_ATF       = 2,
    UPDATEX_FILE_TYPE_TEEOS     = 3,
    UPDATEX_FILE_TYPE_FDT       = 4,
    UPDATEX_FILE_TYPE_UBOOT     = 5,
    UPDATEX_FILE_TYPE_KERNEL    = 6,
    UPDATEX_FILE_TYPE_ROOTFS    = 7,
    UPDATEX_FILE_TYPE_APPFS     = 8,
} UPDATEX_FILE_TYPE_E;

/**
 * 文件信息结构 - 80字节
 */
typedef struct {
    char     filename[MKKIMG_FILENAME_LEN]; /* 文件名 */
    uint32_t file_type;                      /* 文件类型 */
    uint32_t offset;                         /* 文件在整包中的偏移 */
    uint32_t size;                           /* 文件大小（字节） */
    uint32_t crc32;                          /* 文件CRC32校验值 */
} __attribute__((packed)) mkkimg_file_info_t;

/**
 * 镜像头部结构 - 512字节
 */
typedef struct {
    uint32_t magic;                          /* 魔术数: 0x474D494B "KIMG" */
    uint32_t header_version;                 /* 头部版本号 */
    uint32_t file_count;                     /* 打包的文件数量 */
    uint32_t total_size;                     /* 整个镜像文件大小 */
    uint32_t data_offset;                    /* 数据区起始偏移 */
    uint32_t package_crc16;                  /* 整包CRC16校验值 */
    uint32_t create_timestamp;               /* 创建时间戳 */
    uint32_t reserved1;                      /* 保留字段 */

    /* 文件信息数组 */
    mkkimg_file_info_t files[6];             /* 6 * 80 = 480字节 */
} __attribute__((packed)) mkkimg_header_t;

/**
 * 解析并升级mkkimg打包的镜像
 * @param image_path 镜像文件路径（必须是factory.bin）
 * @return 0成功，<0失败
 */
int mkkimg_upgrade_package(const char *image_path);

/**
 * 验证mkkimg镜像完整性
 * @param image_path 镜像文件路径
 * @return 0成功，<0失败
 */
int mkkimg_verify_package(const char *image_path);

/* CRC16和CRC32计算函数 */
uint16_t crc16_ccitt(const uint8_t *data, size_t length);
uint32_t crc32_calc(const uint8_t *data, size_t length);

#endif /* __MKKIMG_UPGRADE_H__ */
