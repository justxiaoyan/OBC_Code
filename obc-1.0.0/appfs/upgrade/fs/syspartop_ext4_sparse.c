/*
 * AM62x EXT4 Sparse Partition Operations
 * EXT4 Sparse格式分区操作接口实现
 * 专门用于处理sparse压缩格式的EXT4文件系统升级
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <mntent.h>
#include "board.h"

/* 检查设备是否已挂载 */
static bool ext4_ismounted(struct obc_syspart *part)
{
    FILE *fp;
    struct mntent *mnt;
    bool mounted = false;

    if (!part || !part->dev) {
        return false;
    }

    fp = setmntent("/proc/mounts", "r");
    if (!fp) {
        return false;
    }

    while ((mnt = getmntent(fp)) != NULL) {
        if (strcmp(mnt->mnt_fsname, part->dev) == 0) {
            mounted = true;
            break;
        }
    }

    endmntent(fp);
    return mounted;
}

/* EXT4文件系统格式化 */
static int ext4_format(struct obc_syspart *part)
{
    int fd;
    char zero_buf[4096] = {0};
    ssize_t ret;
    off_t total_size = 0;

    if (!part || !part->dev) {
        return -1;
    }

    /* 卸载分区（如果已挂载） */
    if (ext4_ismounted(part)) {
        if (part->mnt) {
            umount2(part->mnt, MNT_FORCE | MNT_DETACH);
        }
    }

    /* 清空分区头部数据 */
    fd = open(part->dev, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", part->dev, strerror(errno));
        return -1;
    }

    /* 写0清空分区头部 */
    while (total_size < (2 * 1024 * 1024)) {  /* 清空前2MB */
        ret = write(fd, zero_buf, sizeof(zero_buf));
        if (ret < 0) {
            close(fd);
            return -1;
        }
        total_size += ret;
    }

    fsync(fd);
    close(fd);

    return 0;
}

/* EXT4文件系统挂载 */
static int ext4_mount(struct obc_syspart *part)
{
    int ret;
    struct stat st;

    if (!part || !part->dev || !part->mnt) {
        return -1;
    }

    /* 检查是否已经挂载 */
    if (ext4_ismounted(part)) {
        return 0;
    }

    /* 创建挂载点 */
    if (stat(part->mnt, &st) != 0) {
        if (mkdir(part->mnt, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "Failed to create mount point %s: %s\n",
                    part->mnt, strerror(errno));
            return -1;
        }
    }

    /* 挂载文件系统 */
    ret = mount(part->dev, part->mnt, "ext4",
                MS_MGC_VAL | part->mountflags, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to mount %s to %s: %s\n",
                part->dev, part->mnt, strerror(errno));
        return -1;
    }

    return 0;
}

/* EXT4文件系统卸载 */
static int ext4_umount(struct obc_syspart *part)
{
    if (!part || !part->mnt) {
        return -1;
    }

    /* 检查是否已挂载 */
    if (!ext4_ismounted(part)) {
        return 0;
    }

    /* 强制卸载 */
    return umount2(part->mnt, MNT_FORCE | MNT_DETACH);
}

/* EXT4文件系统同步 */
static int ext4_sync(struct obc_syspart *part)
{
    sync();
    return 0;
}

/* EXT4写分区（裸设备方式） */
static int ext4_write_part(struct obc_syspart *part, char *filename,
                           uint32_t offset, char *buf, int len)
{
    int fd;
    ssize_t ret;
    off_t off_ret;

    if (!part || !part->dev || !buf || len <= 0) {
        return -1;
    }

    /* 写之前需要先卸载 */
    if (ext4_ismounted(part)) {
        ext4_umount(part);
    }

    fd = open(part->dev, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", part->dev, strerror(errno));
        return -1;
    }

    /* 定位到指定偏移 */
    if (offset > 0) {
        off_ret = lseek(fd, offset, SEEK_SET);
        if (off_ret == (off_t)-1) {
            fprintf(stderr, "Failed to seek to offset %u: %s\n",
                    offset, strerror(errno));
            close(fd);
            return -1;
        }
    }

    /* 写入数据 */
    ret = write(fd, buf, len);
    if (ret != len) {
        fprintf(stderr, "Failed to write data: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);

    return ret;
}

/* EXT4读分区（裸设备方式） */
static int ext4_read_part(struct obc_syspart *part, char *filename,
                          uint32_t offset, char *buf, int len)
{
    int fd;
    ssize_t ret;
    off_t off_ret;

    if (!part || !part->dev || !buf || len <= 0) {
        return -1;
    }

    fd = open(part->dev, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", part->dev, strerror(errno));
        return -1;
    }

    /* 定位到指定偏移 */
    if (offset > 0) {
        off_ret = lseek(fd, offset, SEEK_SET);
        if (off_ret == (off_t)-1) {
            fprintf(stderr, "Failed to seek to offset %u: %s\n",
                    offset, strerror(errno));
            close(fd);
            return -1;
        }
    }

    /* 读取数据 */
    ret = read(fd, buf, len);
    if (ret < 0) {
        fprintf(stderr, "Failed to read data: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    return ret;
}

/* EXT4操作接口 */
struct obc_syspart_ops ext4_ops = {
    .format     = ext4_format,
    .ismounted  = ext4_ismounted,
    .mount      = ext4_mount,
    .umount     = ext4_umount,
    .sync       = ext4_sync,
    .write_part = ext4_write_part,
    .read_part  = ext4_read_part,
};
