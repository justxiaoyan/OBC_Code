

#include <stdarg.h>
#include <command.h>
#include <common.h>

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
    int iRet = 0;

    iRet = obc_board_init();
    if (iRet)
    {
        printf("obc board init error\n");
    }

    return iRet;
}

