/*
 * Filesystem Writer Interface
 * 文件系统写入接口定义
 */

#ifndef __FS_WRITER_H__
#define __FS_WRITER_H__

#include <stdint.h>
#include "upgrade.h"

/* 文件系统写入器结构体 */
typedef struct fs_writer {
    const char *name;                   /* 文件系统名称 */
    image_format_t format;              /* 对应的镜像格式 */

    /* 写入接口 */
    int (*write)(const char *image,
                 const partition_info_t *part,
                 uint32_t data_offset,
                 upgrade_progress_cb_t cb);

    /* 验证接口（可选） */
    int (*verify)(const char *image,
                  const partition_info_t *part);
} fs_writer_t;

/* RAW 文件系统写入器 */
extern fs_writer_t fs_writer_raw;

/* EXT4 文件系统写入器 */
extern fs_writer_t fs_writer_ext4;

/* EXT4 Sparse 文件系统写入器 */
extern fs_writer_t fs_writer_ext4_sparse;

/* SquashFS 文件系统写入器 */
extern fs_writer_t fs_writer_squashfs;

/* 获取文件系统写入器 */
fs_writer_t *fs_writer_get(image_format_t format);

#endif /* __FS_WRITER_H__ */
