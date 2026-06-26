



#include <common.h>
#include <stdarg.h>
#include "obc_base_code.h"
#include <command.h>

#include <console.h>
#include <fat.h>
#include <env.h>
#include <mmc.h>
#include <malloc.h>
#include <net.h>
#include <image.h>
#include <init.h>
#include <linux/ctype.h>
#include <board_config.h>


DECLARE_GLOBAL_DATA_PTR;


int do_obcboot(void)
{
    /* 解耦合 standalone 未正常运行，使用 example 例程也启动异常，待排查 */
    //do_go_exec((void *)(long)(0x81000000), 0, NULL);

    int iRet = 0;

    /* 当前使用非解耦合版本 */
    iRet = obc_board_init();
    if (iRet)
    {
        printf("obc board init error\n");
    }

    return iRet;
}



