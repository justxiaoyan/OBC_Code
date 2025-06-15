




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


// 定义结构体
typedef struct OBC_PACK_HEAD
{
    int file_size;
    int crc;
    char pack_file[32];
    char file_name[32];
    char res[512 - 72];
}__attribute__((packed)) OBC_PACK_HEAD_T;


extern void obc_blk_parse_fdt(char *pHostName,const void *fdt, BOARD_ABILITY_TABLE_T *pstAbi);

extern int obc_bootargs_set(BOARD_ABILITY_TABLE_T *pstAbi, char *pDevName, char *pConsole);

extern int obc_fdt_load_to_mem(BOARD_ABILITY_TABLE_T *pstAbi, char *fdtaddr, char *pFdtName);


extern int obc_blk_read_part_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *partname, unsigned int addr);
extern int obc_blk_write_part_by_name(BOARD_ABILITY_TABLE_T *pstBlk, char *pName, unsigned int fileaddr, int file_size);



#endif



