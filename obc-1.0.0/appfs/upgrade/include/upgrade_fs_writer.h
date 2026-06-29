/*
 * Filesystem Writer Interface
 * 文件系统写入接口定义
 */

#ifndef __FS_WRITER_H__
#define __FS_WRITER_H__

#include <stdint.h>
#include "upgrade.h"

/* 文件系统写入器结构体 */
typedef struct FS_WRITER
{
    const char *name;                   /* 文件系统名称 */
    IMAGE_FORMAT_E format;              /* 对应的镜像格式 */

    /* 写入接口 */
    int (*write)(const char *image,
                 const PARTITION_INFO_T *part,
                 uint32_t data_offset,
                 UPGRADE_PROGRESS_CB_T cb);

    /* 验证接口（可选） */
    int (*verify)(const char *image,
                  const PARTITION_INFO_T *part);
}FS_WRITER_T;

/* RAW 文件系统写入器 */
extern FS_WRITER_T fs_writer_raw;

/* EXT4 文件系统写入器 */
extern FS_WRITER_T fs_writer_ext4;

/* EXT4 Sparse 文件系统写入器 */
extern FS_WRITER_T fs_writer_ext4_sparse;

/* SquashFS 文件系统写入器 */
extern FS_WRITER_T fs_writer_squashfs;

/* 获取文件系统写入器 */
FS_WRITER_T *upgrade_fs_writer_get(IMAGE_FORMAT_E format);

#endif /* __FS_WRITER_H__ */
