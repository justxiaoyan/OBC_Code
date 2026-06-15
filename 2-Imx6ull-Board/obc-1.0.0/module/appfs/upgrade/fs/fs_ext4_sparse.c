/*
 * EXT4 Sparse Filesystem Writer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "upgrade.h"
#include "fs_writer.h"

static int ext4_sparse_write(const char *image,
                             const partition_info_t *part,
                             uint32_t data_offset,
                             upgrade_progress_cb_t cb)
{
    /* TODO: 实现稀疏镜像解压 */
    (void)image;
    (void)part;
    (void)data_offset;
    (void)cb;

    fprintf(stderr, "EXT4 sparse image not yet supported\n");
    return -ENOSYS;
}

fs_writer_t fs_writer_ext4_sparse = {
    .name = "ext4-sparse",
    .format = IMAGE_FORMAT_EXT4_SPARSE,
    .write = ext4_sparse_write,
    .verify = NULL,
};
