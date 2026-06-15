/*
 * SquashFS Filesystem Writer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upgrade.h"
#include "fs_writer.h"

/* SquashFS 使用 RAW 写入 */
extern fs_writer_t fs_writer_raw;

static int squashfs_write(const char *image,
                          const partition_info_t *part,
                          uint32_t data_offset,
                          upgrade_progress_cb_t cb)
{
    /* SquashFS 镜像直接使用 RAW 写入 */
    return fs_writer_raw.write(image, part, data_offset, cb);
}

fs_writer_t fs_writer_squashfs = {
    .name = "squashfs",
    .format = IMAGE_FORMAT_SQUASHFS,
    .write = squashfs_write,
    .verify = NULL,
};
