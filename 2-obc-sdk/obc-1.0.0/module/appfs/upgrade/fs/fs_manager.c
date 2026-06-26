/*
 * Filesystem Writer Manager
 */

#include <stdio.h>
#include <string.h>
#include "fs_writer.h"

/* 文件系统写入器列表 */
static fs_writer_t *fs_writers[] = {
    &fs_writer_raw,
    &fs_writer_ext4,
    &fs_writer_ext4_sparse,
    &fs_writer_squashfs,
    NULL
};

/**
 * 根据格式获取写入器
 */
fs_writer_t *fs_writer_get(image_format_t format)
{
    int i;

    for (i = 0; fs_writers[i] != NULL; i++) {
        if (fs_writers[i]->format == format) {
            return fs_writers[i];
        }
    }

    return NULL;
}
