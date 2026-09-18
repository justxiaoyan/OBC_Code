// SPDX-License-Identifier: GPL-2.0+
/*
 * obcflash command - display partition information
 */

#include <common.h>
#include <command.h>
#include <obc_blk.h>
#include <board_config.h>

/* 显示分区表 */
static void display_partition_table(BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int part_count, const char *title)
{
    int i;

    printf("\n%s:\n", title);
    printf("====================================================================================\n");
    printf("%-10s\t%-20s\t%-20s\t%-20s\t%-10s\n", "number:", "partname:", "start:", "size:", "type");

    for (i = 0; i < part_count; i++)
    {
        printf("%-10d\t%-20s\t0x%-18x\t0x%-18x\t%-10s\n",
               i + 1,
               pblkinfo[i].lable,
               pblkinfo[i].addr,
               pblkinfo[i].size,
               pblkinfo[i].flag ? "bootable" : "--");
    }

    printf("====================================================================================\n\n");
}

static int do_obcflash(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    BOARD_ABILITY_TABLE_T *pstAbi = NULL;

    pstAbi = obc_ability_get();

    if (argc < 2)
    {
        printf("Usage: obcflash <-l> [mmc|flash] | obcflash <mmc|flash> erase <partname>\n");
        return CMD_RET_USAGE;
    }

    if (!strcmp(argv[1], "-l"))
    {
        if (argc == 2)
        {
            /* obcflash -l: 显示所有分区信息 */
            if (pstAbi->stBlk.iPartCount_mmc > 0)
            {
                display_partition_table(pstAbi->stBlk.stParts_mmc, pstAbi->stBlk.iPartCount_mmc,
                                       "MMC partition table (obc-mmc)");
            }
            else
            {
                printf("No MMC partition information available\n");
            }

            if (pstAbi->stBlk.iPartCount_flash > 0)
            {
                display_partition_table(pstAbi->stBlk.stParts_flash, pstAbi->stBlk.iPartCount_flash,
                                       "Flash partition table (obc-flash)");
            }
            else
            {
                printf("No Flash partition information available\n");
            }
        }
        else if (argc == 3)
        {
            if (!strcmp(argv[2], "mmc"))
            {
                /* obcflash -l mmc: 显示 obc-mmc 分区信息 */
                if (pstAbi->stBlk.iPartCount_mmc > 0)
                {
                    display_partition_table(pstAbi->stBlk.stParts_mmc, pstAbi->stBlk.iPartCount_mmc,
                                           "MMC partition table (obc-mmc)");
                }
                else
                {
                    printf("No MMC partition information available\n");
                }
            }
            else if (!strcmp(argv[2], "flash"))
            {
                /* obcflash -l flash: 显示 obc-flash 分区信息 */
                if (pstAbi->stBlk.iPartCount_flash > 0)
                {
                    display_partition_table(pstAbi->stBlk.stParts_flash, pstAbi->stBlk.iPartCount_flash,
                                           "Flash partition table (obc-flash)");
                }
                else
                {
                    printf("No Flash partition information available\n");
                }
            }
            else
            {
                printf("Unknown partition type: %s\n", argv[2]);
                return CMD_RET_USAGE;
            }
        }
        else
        {
            printf("Usage: obcflash <-l> [mmc|flash]\n");
            return CMD_RET_USAGE;
        }
    }
    else if (!strcmp(argv[1], "mmc") || !strcmp(argv[1], "flash"))
    {
        /* obcflash <mmc|flash> erase <partname> */
        if (argc == 4 && !strcmp(argv[2], "erase"))
        {
            const char *partname = argv[3];
            int ret;

            /* 根据类型调用对应的擦除函数 */
            if (!strcmp(argv[1], "mmc"))
            {
                ret = obc_blk_erase_mmc_partition(pstAbi, (char *)partname);
            }
            else
            {
                ret = obc_blk_erase_flash_partition(pstAbi, (char *)partname);
            }

            if (0 != ret)
            {
                return CMD_RET_FAILURE;
            }
        }
        else
        {
            printf("Usage: obcflash <mmc|flash> erase <partname>\n");
            return CMD_RET_USAGE;
        }
    }
    else
    {
        printf("Unknown command: %s\n", argv[1]);
        return CMD_RET_USAGE;
    }

    return 0;
}

U_BOOT_CMD(
	obcflash, 4, 0, do_obcflash,
	"display and manage partition information",
	"-l            - display all partition information (mmc + flash)\n"
	"obcflash -l mmc      - display obc-mmc partition information\n"
	"obcflash -l flash    - display obc-flash partition information\n"
	"obcflash mmc erase <partname>   - erase mmc partition\n"
	"obcflash flash erase <partname> - erase flash partition\n"
);
