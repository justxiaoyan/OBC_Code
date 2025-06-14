




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



extern void obc_blk_parse_fdt(char *pHostName,const void *fdt, BOARD_ABILITY_TABLE_T *pstAbi);

extern int obc_bootargs_set(BOARD_ABILITY_TABLE_T *pstAbi, char *pDevName, char *pConsole);

extern int obc_fdt_load_to_mem(BOARD_ABILITY_TABLE_T *pstAbi, char *fdtaddr, char *pFdtName);




#endif



