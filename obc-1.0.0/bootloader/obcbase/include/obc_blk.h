



#ifndef __OBC_BLK_H__
#define __OBC_BLK_H__



#include <bootm.h>
#include <command.h>
#include <image.h>
#include <irq_func.h>
#include <lmb.h>
#include <log.h>
#include <linux/compiler.h>
#include <fdtdec.h>
#include <fdt.h>
#include <board_config.h>
#include <obc_pack.h>
#include <blk.h>
#include <mmc.h>

/* 全局块设备描述符指针 */
extern struct blk_desc *gstBlkDev;

/* 固定使用16K缓冲区大小 */
#define ERASE_BUFFER_SIZE (16 * 1024)

extern void obc_blk_parse_fdt(char *pHostName, const void *fdt, BOARD_ABILITY_TABLE_T *pstAbi);

extern int obc_bootargs_set(BOARD_ABILITY_TABLE_T *pstAbi, char *pDevName, char *pConsole);

extern int obc_fdt_load_to_mem(BOARD_ABILITY_TABLE_T *pstAbi, char *fdtaddr, char *pFdtName);


extern int obc_blk_read_part_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *partname, unsigned int addr);
extern int obc_blk_write_part_by_name(BOARD_ABILITY_TABLE_T *pstBlk, char *pName, unsigned int fileaddr, int file_size);

extern int obc_blk_find_part_by_name_ex(BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int part_count, char *pName);

/* 擦除接口 */
extern int obc_blk_erase_mmc_partition(BOARD_ABILITY_TABLE_T *pstAbi, char *partname);
extern int obc_blk_erase_flash_partition(BOARD_ABILITY_TABLE_T *pstAbi, char *partname);

/* MMC/Flash 特定操作 */
extern int obc_blk_mmc_erase(BOARD_ABILITY_TABLE_T *pstAbi, BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int part_index);
extern int obc_blk_flash_erase(BOARD_ABILITY_TABLE_T *pstAbi, BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int part_index);




#endif


