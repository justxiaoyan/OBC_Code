// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
 */


#include <stdlib.h>
#include <env.h>
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

#include <fdtdec.h>
#include <fdt.h>
#include <obc_blk.h>


/* 升级的目标设备，默认为sd卡：1， emmc：0 */
static int g_iUp_Dev_type = SD_DEV_INDEX;

/* 升级的固件包来源，默认为tftp：UPDATEX_TYPE_TFTP */
static int g_iUp_Src_type = UPDATEX_TYPE_TFTP;


UPDATEX_FW_FILE_LIST_T gst_sd_fw_list[] =
{
    /* file name               文件类型                               升级文件文件格式 raw格式在sd卡中的偏移位置         sd卡中的分区index */
    {UBOOT_FILE_NAME,          UPDATEX_FILE_TYPE_UBOOT,            UPDATEX_FILE_FOMAT_TYPE_RAW,            2,         0},
    {KERNEL_FILE_NAME,         UPDATEX_FILE_TYPE_KERNEL,           UPDATEX_FILE_FOMAT_TYPE_FAT,            0,         1},
    {FDT_FILE_NAME,            UPDATEX_FILE_TYPE_FDT,              UPDATEX_FILE_FOMAT_TYPE_FAT,            0,         1},
    {ROOTFS_FILE_NAME,         UPDATEX_FILE_TYPE_ROOTFS,           UPDATEX_FILE_FOMAT_TYPE_RAW,            11211519,  2},
    {TEE_FILE_NAME,            UPDATEX_FILE_TYPE_TEEOS,            UPDATEX_FILE_FOMAT_TYPE_RAW,            11211519,  2},
    {LOADER_FILE_NAME,         UPDATEX_FILE_TYPE_LOADER,           UPDATEX_FILE_FOMAT_TYPE_RAW,            0,         0},
};

/* 根据文件类型查找文件列表项 */
static UPDATEX_FW_FILE_LIST_T *find_fw_file_by_type(unsigned char file_type)
{
    int i;
    int list_size = sizeof(gst_sd_fw_list) / sizeof(gst_sd_fw_list[0]);

    for (i = 0; i < list_size; i++)
    {
        if (gst_sd_fw_list[i].type == file_type)
        {
            return &gst_sd_fw_list[i];
        }
    }

    printf("Error: file type %d not found in fw list\n", file_type);
    return NULL;
}

int do_updatex_up_getfile(unsigned char file_type, unsigned char up_type, int *file_size)
{
    char command[128] = {0};
    const char *filesize_str = NULL;
    UPDATEX_FW_FILE_LIST_T *fw_file = NULL;
    const char *mmcdev;
    const char *mmcpart;
    const char *alt_name = NULL;

    /* 查找文件类型对应的文件信息 */
    fw_file = find_fw_file_by_type(file_type);
    if (NULL == fw_file)
    {
        printf("Invalid file type: %d\n", file_type);
        return -1;
    }

    /* 1# 匹配升级方式 */
    if (UPDATEX_TYPE_SD == up_type)
    {
        /* The running U-Boot is loaded from TF (mmc1).  Firmware files are
         * placed in its FAT partition and are loaded to the common scratch
         * address before being written to eMMC. */
        mmcdev = env_get("mmcdev");
        mmcpart = env_get("mmcpart");
        if (!mmcdev)
            mmcdev = "1";
        if (!mmcpart)
            mmcpart = "1";
        snprintf(command, sizeof(command), "fatload mmc %s:%s 0x%x %s",
                 mmcdev, mmcpart, UPDATEX_LOADE_ADDR, fw_file->name);
        if (run_command(command, 0) != 0)
        {
            /* Build artifacts use the 100p- prefix while the U-Boot command
             * interface uses the stable *-sign.bin names.  Accept both so a
             * FAT partition can be populated directly from output/image. */
            switch (file_type) {
            case UPDATEX_FILE_TYPE_LOADER: alt_name = "100p-loader.bin"; break;
            case UPDATEX_FILE_TYPE_FDT:    alt_name = "100p-fdt.bin"; break;
            case UPDATEX_FILE_TYPE_TEEOS:  alt_name = "100p-teeos.bin"; break;
            case UPDATEX_FILE_TYPE_KERNEL: alt_name = "100p-kernel.bin"; break;
            case UPDATEX_FILE_TYPE_ROOTFS: alt_name = "100p-rootfs.bin"; break;
            case UPDATEX_FILE_TYPE_UBOOT:  alt_name = "100p-uboot.bin"; break;
            default: break;
            }
            if (!alt_name) {
                printf("Failed to load %s from TF FAT partition\n", fw_file->name);
                return -1;
            }
            snprintf(command, sizeof(command), "fatload mmc %s:%s 0x%x %s",
                     mmcdev, mmcpart, UPDATEX_LOADE_ADDR, alt_name);
            if (run_command(command, 0) != 0) {
                printf("Failed to load %s or %s from TF FAT partition\n",
                       fw_file->name, alt_name);
                return -1;
            }
            printf("Loaded compatibility artifact %s\n", alt_name);
        }

        filesize_str = env_get("filesize");
        if (!filesize_str)
        {
            printf("fatload did not set filesize for %s\n", fw_file->name);
            return -1;
        }
        *file_size = simple_strtoul(filesize_str, NULL, 16);
        printf("Loaded %s from TF (%d bytes)\n", fw_file->name, *file_size);
    }
    else if (UPDATEX_TYPE_TFTP == up_type)
    {
        /* #2 获取升级文件，tftp get file */
        sprintf(command, "tftp %x %s", UPDATEX_LOADE_ADDR, fw_file->name);
        if (run_command(command, 0) != 0)
            return -1;

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

int do_updatex_sd_writefile(unsigned char file_type, int file_size)
{
    int write_cnt = 0;
    char command[128] = {0};
    UPDATEX_FW_FILE_LIST_T *fw_file = find_fw_file_by_type(file_type);

    if (!fw_file)
        return -1;

    /* 1# 判断升级方式 */
    if ((UPDATEX_FILE_TYPE_UBOOT == file_type)
        || (UPDATEX_FILE_TYPE_ROOTFS == file_type))
    {
        /* cale block cnt */
        write_cnt = (file_size / UPDATEX_BLOCK_SIZE) + 1;

        /* select src dev */
        sprintf(command, "mmc dev 0");
        run_command(command, 0);

        memset(command, 0, sizeof(command));
        sprintf(command, "mmc write %x %x %x", UPDATEX_LOADE_ADDR, fw_file->start_sector, write_cnt);
        if (run_command(command, 0) != 0)
            return -1;

        printf("updatex write %s success\n", fw_file->name);
    }
    else if ((UPDATEX_FILE_TYPE_FDT == file_type)
            || (UPDATEX_FILE_TYPE_KERNEL == file_type))
    {
        sprintf(command, "fatrm mmc %d:%d %s", SD_DEV_INDEX, fw_file->part_index,
                                                             fw_file->name);
        run_command(command, 0);

        sprintf(command, "fatwrite mmc %d:%d %x %s %x", SD_DEV_INDEX,
                                                        fw_file->part_index,
                                                        UPDATEX_LOADE_ADDR,
                                                        fw_file->name,
                                                        file_size);
        if (run_command(command, 0) != 0)
            return -1;
    }

    return 0;
}

int do_updatex_mmc_write(char *pName, int file_size)
{
    BOARD_ABILITY_TABLE_T * pstAbi = NULL;

    pstAbi = obc_ability_get();

    /* Select eMMC user area as the destination.  The board starts from TF,
     * so the descriptor initialized during board init points at mmc1. */
    gstBlkDev = mmc_get_blk_desc(find_mmc_device(EMMC_DEV_INDEX));
    if (!gstBlkDev)
    {
        printf("eMMC block device is not available\n");
        return -1;
    }

    return obc_blk_write_part_by_name(pstAbi, pName, UPDATEX_LOADE_ADDR, file_size);
}


int do_updatex_emmc_head_check(void)
{
    /* 裸分区模式，不再检查 OBCFS 头部 */
    return 0;
}


int do_updatex_sd_raw_writefile(unsigned char file_type, int file_size)
{
    /* 裸分区模式，所有固件类型都直接写入，不检查头部 */
    if (UPDATEX_FILE_TYPE_LOADER == file_type)
    {
        do_updatex_mmc_write("loader0", file_size);
        do_updatex_mmc_write("loader1", file_size);
    }
    else if (UPDATEX_FILE_TYPE_FDT == file_type)
    {
        do_updatex_mmc_write("fdt0", file_size);
        do_updatex_mmc_write("fdt1", file_size);
        do_updatex_mmc_write("fdt2", file_size);
    }
    else if (UPDATEX_FILE_TYPE_KERNEL == file_type)
    {
        do_updatex_mmc_write("kernel0", file_size);
        do_updatex_mmc_write("kernel1", file_size);
        do_updatex_mmc_write("kernel2", file_size);
    }
    else if (UPDATEX_FILE_TYPE_UBOOT == file_type)
    {
        do_updatex_mmc_write("uboot0", file_size);
        do_updatex_mmc_write("uboot1", file_size);
        do_updatex_mmc_write("uboot2", file_size);
    }
    else if (UPDATEX_FILE_TYPE_TEEOS == file_type)
    {
        do_updatex_mmc_write("teeos0", file_size);
        do_updatex_mmc_write("teeos1", file_size);
        do_updatex_mmc_write("teeos2", file_size);
    }
    else if (UPDATEX_FILE_TYPE_ROOTFS == file_type)
    {
        do_updatex_mmc_write("rootfs0", file_size);
        do_updatex_mmc_write("rootfs1", file_size);
        do_updatex_mmc_write("rootfs2", file_size);
    }
    else
    {
        printf("unknown file type %d\n", file_type);
        return -1;
    }

    return 0;
}


int do_updatex_emmc_writefile(unsigned char file_type, int file_size)
{
    int write_cnt = 0;
    char command[128] = {0};

    /* 裸分区模式，所有固件类型都直接写入，不检查头部 */
    if (UPDATEX_FILE_TYPE_UBOOT == file_type)
    {
        /* boot0分区切换 */
        /* cale block cnt */
        write_cnt = (file_size / UPDATEX_BLOCK_SIZE) + 1;

        /* select src dev boot0 */
        sprintf(command, "mmc dev %d", EMMC_DEV_INDEX);
        if (run_command(command, 0) != 0)
            return -1;

        memset(command, 0, sizeof(command));
        sprintf(command, "mmc dev %d 1", EMMC_DEV_INDEX);        /* BOOT0 */
        if (run_command(command, 0) != 0)
            return -1;

        memset(command, 0, sizeof(command));
        sprintf(command, "mmc write %x 2 %x", UPDATEX_LOADE_ADDR, write_cnt);
        if (run_command(command, 0) != 0)
            return -1;

        memset(command, 0, sizeof(command));
        sprintf(command, "mmc dev %d", EMMC_DEV_INDEX);
        run_command(command, 0);
    }
    else if (UPDATEX_FILE_TYPE_LOADER == file_type)
    {
        if (do_updatex_mmc_write("loader0", file_size) != 0 ||
            do_updatex_mmc_write("loader1", file_size) != 0)
            return -1;
    }
    else if (UPDATEX_FILE_TYPE_FDT == file_type)
    {
        if (do_updatex_mmc_write("fdt0", file_size) != 0 ||
            do_updatex_mmc_write("fdt1", file_size) != 0 ||
            do_updatex_mmc_write("fdt2", file_size) != 0)
            return -1;
    }
    else if (UPDATEX_FILE_TYPE_KERNEL == file_type)
    {
        if (do_updatex_mmc_write("kernel0", file_size) != 0 ||
            do_updatex_mmc_write("kernel1", file_size) != 0 ||
            do_updatex_mmc_write("kernel2", file_size) != 0)
            return -1;
    }
    else if (UPDATEX_FILE_TYPE_TEEOS == file_type)
    {
        if (do_updatex_mmc_write("teeos0", file_size) != 0 ||
            do_updatex_mmc_write("teeos1", file_size) != 0 ||
            do_updatex_mmc_write("teeos2", file_size) != 0)
            return -1;
    }
    else if (UPDATEX_FILE_TYPE_ROOTFS == file_type)
    {
        if (do_updatex_mmc_write("rootfs0", file_size) != 0 ||
            do_updatex_mmc_write("rootfs1", file_size) != 0 ||
            do_updatex_mmc_write("rootfs2", file_size) != 0)
            return -1;
    }

    return 0;
}


int do_updatex_up(unsigned char file_type, unsigned char up_type)
{
    int ret = 0;
    int file_size = 0;

    /* 1# get file */
    ret = do_updatex_up_getfile(file_type, up_type, &file_size);
    if (ret)
    {
        printf("updatex up get file failed ret %d\n", ret);
        return -1;
    }

    /* 2# write file */
    if (EMMC_DEV_INDEX == g_iUp_Dev_type)
    {
        ret = do_updatex_emmc_writefile(file_type, file_size);
        if (ret)
        {
            printf("updatex emmc write file failed ret %d\n", ret);
        }
    }
    else if (SD_DEV_INDEX == g_iUp_Dev_type)
    {
        printf("Writing upgrade images to SD is not supported; select eMMC with -d emmc\n");
        return -1;
    }
    else
    {
        printf("update dev type invalid %d\n", g_iUp_Dev_type);
        return -1;
    }


    return ret;
}


static int do_updatex(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;
    unsigned char file_type = UPDATEX_FILE_TYPE_NONE;
    unsigned char up_type = g_iUp_Src_type;

    /* 参数不足，显示帮助 */
    if (argc < 2)
    {
        return CMD_RET_USAGE;
    }

    /* 1# switch cmd */
    if (argc > 2)
    {
        /* cmd type */
        if (!strcmp(argv[1], "-d"))
        {
            /* updatex -d <sd/emmc/flash>: 配置升级目标设备 */
            if (!strcmp(argv[2], "sd"))
            {
                g_iUp_Dev_type = SD_DEV_INDEX;
                printf("Set upgrade target device: SD (dev %d)\n", g_iUp_Dev_type);
            }
            else if (!strcmp(argv[2], "emmc"))
            {
                g_iUp_Dev_type = EMMC_DEV_INDEX;
                printf("Set upgrade target device: EMMC (dev %d)\n", g_iUp_Dev_type);
            }
            else if (!strcmp(argv[2], "flash"))
            {
                printf("Flash device not supported yet\n");
                return CMD_RET_FAILURE;
            }
            else
            {
                printf("Unknown device type: %s\n", argv[2]);
                return CMD_RET_USAGE;
            }
            return 0;
        }
        else if (!strcmp(argv[1], "-s"))
        {
            /* updatex -s <tftp/sd>: 配置升级固件包来源 */
            if (!strcmp(argv[2], "tftp"))
            {
                g_iUp_Src_type = UPDATEX_TYPE_TFTP;
                printf("Set upgrade source: TFTP\n");
            }
            else if (!strcmp(argv[2], "sd"))
            {
                g_iUp_Src_type = UPDATEX_TYPE_SD;
                printf("Set upgrade source: SD card\n");
                printf("Warning: SD card upgrade source not fully implemented yet\n");
            }
            else
            {
                printf("Unknown source type: %s\n", argv[2]);
                return CMD_RET_USAGE;
            }
            return 0;
        }
        else if (!strcmp(argv[1], "up"))
        {
            /* updatex up <uboot/kernel/fdt/tee/all>: 升级命令 */
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
            else if (!strcmp(argv[2], "tee"))
            {
                file_type = UPDATEX_FILE_TYPE_TEEOS;
            }
            else if (!strcmp(argv[2], "teeos"))
            {
                file_type = UPDATEX_FILE_TYPE_TEEOS;
            }
            else if (!strcmp(argv[2], "loader"))
            {
                file_type = UPDATEX_FILE_TYPE_LOADER;
            }
            else if (!strcmp(argv[2], "rootfs"))
            {
                file_type = UPDATEX_FILE_TYPE_ROOTFS;
            }
            else if (!strcmp(argv[2], "all"))
            {
                const unsigned char all_types[] = {
                    UPDATEX_FILE_TYPE_LOADER, UPDATEX_FILE_TYPE_FDT,
                    UPDATEX_FILE_TYPE_TEEOS, UPDATEX_FILE_TYPE_KERNEL,
                    UPDATEX_FILE_TYPE_ROOTFS,
                };
                unsigned int i;
                for (i = 0; i < sizeof(all_types); i++)
                {
                    ret = do_updatex_up(all_types[i], up_type);
                    if (ret)
                        return CMD_RET_FAILURE;
                }
                return 0;
            }
            else
            {
                printf("Unknown file type: %s\n", argv[2]);
                return CMD_RET_USAGE;
            }
        }
        else
        {
            printf("Unknown command: %s\n", argv[1]);
            return CMD_RET_USAGE;
        }
    }
    else
    {
        /* 参数不足 */
        return CMD_RET_USAGE;
    }

    /* 检查 file_type 是否有效 */
    if (UPDATEX_FILE_TYPE_NONE == file_type)
    {
        printf("Error: No valid file type specified\n");
        return CMD_RET_USAGE;
    }

    /* 2# update */
    ret = do_updatex_up(file_type, up_type);
    if (ret)
    {
        printf("updatex up failed %d\n", ret);
    }

    return 0;
}


U_BOOT_CMD(
	updatex, 5, 0, do_updatex,
	"firmware upgrade tool",
	"-d <sd|emmc|flash>  - set upgrade target device (default: sd)\n"
	"updatex -s <tftp|sd>        - set upgrade source (default: tftp)\n"
	"updatex up <loader|fdt|teeos|kernel|rootfs|uboot|all> - upgrade firmware\n"
);


static int do_upfdt(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_FDT, g_iUp_Src_type);
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

    ret = do_updatex_up(UPDATEX_FILE_TYPE_KERNEL, g_iUp_Src_type);
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


static int do_uprootfs(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_ROOTFS, g_iUp_Src_type);
    if (ret)
    {
        printf("uprootfs failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	uprootfs, 1, 0, do_uprootfs,
	"upgrade rootfs0/1 partitions",
	"upgrade rootfs0/1 partitions via tftp\n"
);


static int do_uploader(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_LOADER, g_iUp_Src_type);
    if (ret)
    {
        printf("uploader failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	uploader, 1, 0, do_uploader,
	"upgrade loader0/1 partitions",
	"upgrade loader0/1 partitions via tftp\n"
);


static int do_upteeos(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_TEEOS, g_iUp_Src_type);
    if (ret)
    {
        printf("upteeos failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	upteeos, 1, 0, do_upteeos,
	"upgrade teeos0/1 partitions",
	"upgrade teeos0/1 partitions via tftp\n"
);


static int do_upuboot(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_UBOOT, g_iUp_Src_type);
    if (ret)
    {
        printf("upuboot failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	upuboot, 1, 0, do_upuboot,
	"upgrade uboot0/1 partitions",
	"upgrade uboot0/1 partitions via tftp\n"
);


static int do_upkernel(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;

    ret = do_updatex_up(UPDATEX_FILE_TYPE_KERNEL, g_iUp_Src_type);
    if (ret)
    {
        printf("upkernel failed %d\n", ret);
    }

    return 0;
}

U_BOOT_CMD(
	upkernel, 1, 0, do_upkernel,
	"upgrade kernel0/1 partitions",
	"upgrade kernel0/1 partitions via tftp\n"
);
