/*
 * EXT4 Filesystem Writer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upgrade.h"
#include "fs_writer.h"

/* EXT4 使用 RAW 写入 */
extern fs_writer_t fs_writer_raw;

static int ext4_write(const char *image,
                      const partition_info_t *part,
                      uint32_t data_offset,
                      upgrade_progress_cb_t cb)
{
    /* EXT4 镜像直接使用 RAW 写入 */
    return fs_writer_raw.write(image, part, data_offset, cb);
}

fs_writer_t fs_writer_ext4 = {
    .name = "ext4",
    .format = IMAGE_FORMAT_EXT4,
    .write = ext4_write,
    .verify = NULL,
};
