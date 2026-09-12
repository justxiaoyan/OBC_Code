
#include <obc_blk.h>
#include <mmc.h>
#include <blk.h>
#include <malloc.h>

/*
 * MMC/SD 特定操作实现
 * 当前 MMC/SD 的读写操作已在 obc_blk.c 中通过 blk_dread/blk_dwrite 实现
 * 此文件预留用于未来 MMC 特定的优化或扩展功能
 */

/* 预留：MMC 特定的初始化函数 */
int obc_blk_mmc_init(void)
{
    /* 当前不需要特殊初始化 */
    return 0;
}

/* MMC 特定的擦除函数 */
int obc_blk_mmc_erase(BOARD_ABILITY_TABLE_T *pstAbi, BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int part_index)
{
    ulong start, count;
    ulong ulCnt;
    unsigned char *zero_buf;
    int ret = 0;
    ulong remaining_count;
    ulong current_start;
    ulong write_count;

    if (NULL == gstBlkDev)
    {
        printf("Error: Block device not initialized\n");
        return -1;
    }

    /* 计算擦除的块数量 */
    start = pblkinfo[part_index].addr / pstAbi->stBlk.iRdSize;
    count = pblkinfo[part_index].size / pstAbi->stBlk.iRdSize;

    printf("MMC Erase: partition=%s, start=0x%lx, count=0x%lx (size=0x%x)\n",
           pblkinfo[part_index].lable, start, count, pblkinfo[part_index].size);

    /* 分配固定4K缓冲区，避免大分区malloc失败 */
    zero_buf = (unsigned char *)malloc(ERASE_BUFFER_SIZE);
    if (NULL == zero_buf)
    {
        printf("Error: Failed to allocate 4K erase buffer\n");
        return -1;
    }

    /* 填充零 */
    memset(zero_buf, 0, ERASE_BUFFER_SIZE);

    /* 计算每次写入的块数 */
    ulong blocks_per_write = ERASE_BUFFER_SIZE / pstAbi->stBlk.iRdSize;
    if (blocks_per_write == 0)
    {
        printf("Error: Block size (%d) larger than buffer size (%d)\n",
               pstAbi->stBlk.iRdSize, ERASE_BUFFER_SIZE);
        free(zero_buf);
        return -1;
    }

    /* 分块写入零数据 */
    remaining_count = count;
    current_start = start;

    while (remaining_count > 0)
    {
        /* 计算本次写入的块数 */
        write_count = (remaining_count > blocks_per_write) ? blocks_per_write : remaining_count;

        /* 写入零数据 */
        ulCnt = blk_dwrite(gstBlkDev, current_start, write_count, (void *)zero_buf);
        if (write_count != ulCnt)
        {
            printf("Error: Failed to erase MMC partition %s at block 0x%lx (wrote %lu/%lu blocks)\n",
                   pblkinfo[part_index].lable, current_start, ulCnt, write_count);
            ret = -1;
            break;
        }

        /* 更新进度 */
        remaining_count -= write_count;
        current_start += write_count;

        /* 显示擦除进度（每擦除1MB显示一次） */
        if (((count - remaining_count) % (1024 * 1024 / pstAbi->stBlk.iRdSize)) == 0)
        {
            printf("Erasing... %lu%% (%lu/%lu blocks)\n",
                   ((count - remaining_count) * 100) / count,
                   count - remaining_count, count);
        }
    }

    if (ret == 0)
    {
        printf("MMC partition %s erased successfully (total %lu blocks)\n",
               pblkinfo[part_index].lable, count);
    }

    /* 释放缓冲区 */
    free(zero_buf);

    return ret;
}
