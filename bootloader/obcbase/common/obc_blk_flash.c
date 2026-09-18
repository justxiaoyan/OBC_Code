
#include <obc_blk.h>

/*
 * Flash 特定操作实现
 * 此文件预留用于未来 Flash 存储介质的支持
 * 当前所有操作返回未实现错误
 */

/* 预留：Flash 特定的初始化函数 */
int obc_blk_flash_init(void)
{
    printf("Flash operations not implemented yet\n");
    return -1;
}

/* 预留：Flash 特定的读取函数 */
int obc_blk_flash_read(BOARD_ABILITY_TABLE_T *pstAbi, char *pName, unsigned int addr)
{
    printf("Flash read not implemented yet\n");
    return -1;
}

/* 预留：Flash 特定的写入函数 */
int obc_blk_flash_write(BOARD_ABILITY_TABLE_T *pstAbi, char *pName, unsigned int fileaddr, int file_size)
{
    printf("Flash write not implemented yet\n");
    return -1;
}

/* Flash 特定的擦除函数 */
int obc_blk_flash_erase(BOARD_ABILITY_TABLE_T *pstAbi, BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int part_index)
{
    printf("Flash erase not implemented yet\n");
    printf("Partition: %s, addr=0x%x, size=0x%x\n",
           pblkinfo[part_index].lable,
           pblkinfo[part_index].addr,
           pblkinfo[part_index].size);
    return -1;
}

