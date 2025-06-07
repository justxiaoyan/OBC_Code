// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
 */

//#include <pdm_imx6ull_config.h>
#include <stdlib.h>

#include <blk.h>
#include <command.h>
#include <console.h>
#include <display_options.h>
#include <memalign.h>
#include <mmc.h>
#include <part.h>
#include <sparse_format.h>
#include <image-sparse.h>
#include <vsprintf.h>

#include <cmd_updatex.h>


UPDATEX_FW_FILE_LIST_T gst_sd_fw_list[] = 
{
    /* file name               文件类型                               文件格式                                   sd卡中的分区index */
    {"file name",              UPDATEX_FILE_TYPE_NONE,             UPDATEX_FILE_FOMAT_TYPE_RAW,            0},
    {UBOOT_FILE_NAME,          UPDATEX_FILE_TYPE_UBOOT,            UPDATEX_FILE_FOMAT_TYPE_RAW,            0},
    {KERNEL_FILE_NAME,         UPDATEX_FILE_TYPE_KERNEL,           UPDATEX_FILE_FOMAT_TYPE_FAT,            1},
    {FDT_FILE_NAME,            UPDATEX_FILE_TYPE_FDT,              UPDATEX_FILE_FOMAT_TYPE_FAT,            1}
};

int do_updatex_up_getfile(unsigned char file_type, unsigned char up_type, int *file_size)
{
    char command[128] = {0};
    const char *filesize_str =NULL;

    /* 1# 匹配升级方式 */
    if (UPDATEX_TYPE_SD == up_type)
    {
    #if 0
        for (i = 0; i < ()/(); i++)
        {
            /* read file info from sd card*/
            do_read_info_from_sd(buffer, offset, size);
        }

        offset += size;
        #endif
    }
    else if (UPDATEX_TYPE_TFTP == up_type)
    {
        /* #2 获取升级文件，tftp get file */
        sprintf(command, "tftp %x %s", UPDATEX_LOADE_ADDR, gst_sd_fw_list[file_type].name);
        run_command(command, 0);

        /* 3# get file size */
        filesize_str = env_get("filesize");
        if (filesize_str)
        {
            *file_size = simple_strtoul(filesize_str, NULL, 16);
            printf("File size: %d bytes\n", *file_size);
        }
        else
        {
            printf("Failed to get file size\n");
            return -1;
        }
    }

    return 0;
}

int do_updatex_up_writefile(unsigned char file_type, int file_size)
{
    int write_cnt = 0;
    char command[128] = {0};

    /* 1# 判断升级方式 */
    if (UPDATEX_FILE_TYPE_UBOOT == file_type)
    {
        /* cale block cnt */
        write_cnt = (file_size / UPDATEX_BLOCK_SIZE) + 1;
        
        /* select src dev */
        sprintf(command, "mmc dev 0");
        run_command(command, 0);

        memset(command, 0, sizeof(command));
        sprintf(command, "mmc write %x 2 %x", UPDATEX_LOADE_ADDR, write_cnt);
        run_command(command, 0);
    }
    else if ((UPDATEX_FILE_TYPE_FDT == file_type)
            || (UPDATEX_FILE_TYPE_KERNEL == file_type))
    {
        sprintf(command, "fatrm mmc %d:%d %s", SD_DEV_INDEX, gst_sd_fw_list[file_type].sd_part_index,
                                                             gst_sd_fw_list[file_type].name);
        run_command(command, 0);

        sprintf(command, "fatwrite mmc %d:%d %x %s %x", SD_DEV_INDEX,
                                                        gst_sd_fw_list[file_type].sd_part_index,
                                                        UPDATEX_LOADE_ADDR,
                                                        gst_sd_fw_list[file_type].name,
                                                        file_size);
        run_command(command, 0);
    }

    return 0;
}

int do_updatex_up(unsigned char file_type, unsigned char up_type)
{
    int ret = 0;
    int i = 0;
    int offset, file_size = 0;

    /* 1# get file */
    ret = do_updatex_up_getfile(file_type, up_type, &file_size);
    if (ret)
    {
        printf("updatex up get file failed ret %d\n", ret);
    }

    /* 2# write file */
    ret = do_updatex_up_writefile(file_type, file_size);
    if (ret)
    {
        printf("updatex up write file failed ret %d\n", ret);
    }

    return 0;
}


static int do_updatex(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;
    unsigned char file_type, up_type = 0;

    /* 1# switch cmd */
    if (argc > 2)
    {
        /* cmd type */
        if (!strcmp(argv[1], "dev"))
        {

        }
        else if (!strcmp(argv[1], "src"))
        {

        }
        else if (!strcmp(argv[1], "up"))
        {
            /* updatex file type */
            if (!strcmp(argv[2], "uboot"))
            {
                file_type = UPDATEX_FILE_TYPE_UBOOT;
            }
            else if (!strcmp(argv[2], "kernel"))
            {
                file_type = UPDATEX_FILE_TYPE_KERNEL;
            }
            else if (!strcmp(argv[2], "fdt"))
            {
                file_type = UPDATEX_FILE_TYPE_FDT;
            }
            else if (!strcmp(argv[2], "all"))
            {
            
            }
        }
    }

    /* 2# select up type */
    up_type = UPDATEX_TYPE_TFTP;

    /* 3# update */
    ret = do_updatex_up(file_type, up_type);
    if (ret)
    {
        printf("updatex up failed %d\n", ret);
    }

    return 0;
}


U_BOOT_CMD(
	updatex, 5, 0, do_updatex,
	"display updatex info",
	"- updatex dev <emmc/sd/nor>\n"
	"- updatex src <sd/tftp>\n"
	"- updatex up <uboot/kernel/all>\n"
);


static int do_upb(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_UBOOT, UPDATEX_TYPE_TFTP);
    if (ret)
    {
        printf("upb failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	upb, 1, 0, do_upb,
	"updatex up <uboot/kernel/all>",
	"updatex up <uboot/kernel\n"
);

static int do_upfdt(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_FDT, UPDATEX_TYPE_TFTP);
    if (ret)
    {
        printf("upfdt failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	upfdt, 1, 0, do_upfdt,
	"updatex up <uboot/kernel/all>",
	"updatex up <uboot/kernel\n"
);


static int do_upk(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_KERNEL, UPDATEX_TYPE_TFTP);
    if (ret)
    {
        printf("upk failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	upk, 1, 0, do_upk,
	"updatex up <uboot/kernel/all>",
	"updatex up <uboot/kernel\n"
);













