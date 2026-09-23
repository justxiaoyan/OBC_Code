


#include <board_config_am62x.h>
#include <obc_blk.h>
#include <asm/arch/am62_spl.h>
#include <asm/arch/hardware.h>


int am62x_board_hw_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    u32 rom_bootdev = get_boot_device();
    struct mmc *mmc;
    int bootdev;
    int ret;

    switch (rom_bootdev)
    {
    case BOOT_DEVICE_MMC1:
        bootdev = CONFIG_AM62X_DEV_EMMC;
        break;
    case BOOT_DEVICE_MMC2:
    case BOOT_DEVICE_MMC2_2:
        bootdev = CONFIG_AM62X_DEV_SD;
        break;
    default:
        printf("OBC: unsupported ROM boot device 0x%x, defaulting to eMMC\n",
               rom_bootdev);
        bootdev = CONFIG_AM62X_DEV_EMMC;
        break;
    }

    printf("OBC: boot media is %s (U-Boot mmc %d)\n",
           bootdev == CONFIG_AM62X_DEV_EMMC ? "eMMC" : "SD", bootdev);

    /* 1# 启动设备内容获取 */
    pstAbi->stBoot.iMediaId = bootdev;
    if (CONFIG_AM62X_DEV_SD == bootdev)
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_SD;
    else if (CONFIG_AM62X_DEV_EMMC == bootdev)
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_EMMC;

    /* 2# MMC控制器信息 */
    pstAbi->stBlk.iRdSize = 512;
    mmc = find_mmc_device(bootdev);
    if (NULL == mmc)
    {
        printf("OBC: MMC device %d is not available\n", bootdev);
        return -1;
    }

    ret = mmc_init(mmc);
    if (ret != 0)
    {
        printf("OBC: failed to initialize MMC device %d, ret=%d\n",
               bootdev, ret);
        return ret;
    }

    ret = blk_select_hwpart_devnum(UCLASS_MMC, bootdev,
                                   EMMC_HWPART_DEFAULT);
    if (ret != 0)
    {
        printf("OBC: failed to select MMC device %d user area, ret=%d\n",
               bootdev, ret);
        return ret;
    }

    gstBlkDev = mmc_get_blk_desc(mmc);

    if (NULL == gstBlkDev)
    {
        printf("OBC: failed to get block descriptor for MMC device %d\n",
               bootdev);
        return -1;
    }

    pstAbi->stBoot.iPartType = OBC_SLOT_A;
    if (obc_slot_read(&pstAbi->stBoot.iPartType) != 0)
        printf("OBC: sysflag unavailable, defaulting to slot A\n");
    else
        printf("OBC: selected boot slot %c\n", 'A' + pstAbi->stBoot.iPartType);

    /* 3# 启动地址 */
    pstAbi->stBlk.def_fdt = pstAbi->stBoot.iPartType == OBC_SLOT_B ?
                            CONFIG_DEFAULT_AM62X_FDT1_DEVADDR :
                            CONFIG_DEFAULT_AM62X_FDT0_DEVADDR;
    pstAbi->stBoot.uiFdtAddr = CONFIG_AM62X_HEX_FDT_ADDR;
    pstAbi->stBoot.uiImageAddr = CONFIG_AM62X_HEX_KERNEL_ADDR;
    pstAbi->stBoot.uiTmpAddr = 0x84000000;

    return 0;
}

/* 环境变量初始化 */
int am62x_board_env_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    const char *env_value;
    const char *mmcdev = pstAbi->stBoot.iBootMedia == BOARD_ABILITY_DEV_EMMC ? "0" : "1";
    int env_changed = 0;

    if (NULL == env_get("bootdelay"))
    {
        env_set("netmask", "255.255.0.0");

        env_set("fdt_addr", CONFIG_AM62X_STR_FDT_ADDR);
        env_set("loadaddr", CONFIG_AM62X_STR_KERNEL_ADDR);

        env_set("mmcpart", "1");

        env_set("bootdelay", "1");
        env_changed = 1;
    }

    env_value = env_get("serverip");
    if ((NULL == env_value) || (0 != strcmp(env_value, "192.168.1.124")))
    {
        env_set("serverip", "192.168.1.124");
        env_changed = 1;
    }

    env_value = env_get("ipaddr");
    if ((NULL == env_value) || (0 != strcmp(env_value, "192.168.1.100")))
    {
        env_set("ipaddr", "192.168.1.50");
        env_changed = 1;
    }

    env_value = env_get("gatewayip");
    if ((NULL == env_value) || (0 != strcmp(env_value, "192.168.1.1")))
    {
        env_set("gatewayip", "192.168.1.1");
        env_changed = 1;
    }

    env_value = env_get("mmcdev");
    if ((NULL == env_value) || (0 != strcmp(env_value, mmcdev)))
    {
        env_set("mmcdev", mmcdev);
        env_changed = 1;
    }

    if ((BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
        || (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia))
    {
        env_value = env_get("mmcboot");
        if ((NULL == env_value) || (0 != strcmp(env_value, "bootk")))
        {
            env_set("mmcboot", "bootk");
            env_changed = 1;
        }

        env_value = env_get("bootcmd");
        if ((NULL == env_value) || (0 != strcmp(env_value, "run mmcboot")))
        {
            env_set("bootcmd", "run mmcboot");
            env_changed = 1;
        }
    }

    if (env_changed)
    {
        env_save();
    }

    return 0;
}

int am62x_board_fdt_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    int iRet = 0;
    void *fdt_addr = (void *)CONFIG_AM62X_HEX_FDT_ADDR;
    BOARD_ABILITY_TABLE_T *spl_shared_ability = (BOARD_ABILITY_TABLE_T *)CONFIG_OBC_SHMEM_ADDR;

    /* 检查 SPL 阶段是否已经解析了分区信息 */
    if (memcmp(spl_shared_ability->magic, "OBCFS", 5) == 0)
    {
        printf("U-Boot: Using SPL parsed partition information\n");

        /* 使用 SPL 阶段解析的分区信息 */
        pstAbi->stBlk.iPartCount_mmc = spl_shared_ability->stBlk.iPartCount_mmc;
        pstAbi->stBlk.iPartCount_flash = spl_shared_ability->stBlk.iPartCount_flash;
		pstAbi->stBoot.iPartType = spl_shared_ability->stBoot.iPartType;

        memcpy(pstAbi->stBlk.stParts_mmc, spl_shared_ability->stBlk.stParts_mmc,
               sizeof(pstAbi->stBlk.stParts_mmc));
        memcpy(pstAbi->stBlk.stParts_flash, spl_shared_ability->stBlk.stParts_flash,
               sizeof(pstAbi->stBlk.stParts_flash));

        /*
         * SPL 只负责通过共享内存传递已解析的分区表。U-Boot 仍在自己的
         * FDT 目标地址重新读取并校验 fdt0/fdt1，避免依赖 SPL 阶段的
         * FDT 缓冲区内容，也避免两个阶段共享同一份可能被覆盖的数据。
         */
        iRet = obc_fdt_load_to_mem(pstAbi, CONFIG_AM62X_STR_FDT_ADDR,
                                    CONFIG_AM62X_FDT_NAME);
        if (0 != iRet)
        {
            printf("U-Boot: fdt load to mem error\n");
            obc_slot_switch(pstAbi->stBoot.iPartType);
            return -1;
        }

        return 0;
    }

    /* SPL 阶段没有有效数据，走原有流程 */
    printf("U-Boot: No valid SPL data found, using original flow\n");

    /* 1# 加载设备树,检查设备树是否有效 */
    iRet = obc_fdt_load_to_mem(pstAbi, CONFIG_AM62X_STR_FDT_ADDR, CONFIG_AM62X_FDT_NAME);
    if  (0 != iRet)
    {
        printf("fdt load to mem error\n");
        obc_slot_switch(pstAbi->stBoot.iPartType);
        return -1;
    }

    /* 2# 清空分区信息，准备重新解析设备树 */
    pstAbi->stBlk.iPartCount_mmc = 0;
    pstAbi->stBlk.iPartCount_flash = 0;
    memset((void *)&pstAbi->stBlk.stParts_mmc[0], 0, sizeof(pstAbi->stBlk.stParts_mmc));
    memset((void *)&pstAbi->stBlk.stParts_flash[0], 0, sizeof(pstAbi->stBlk.stParts_flash));

    /* 3# 解析设备树填充 obc-mmc 分区信息 */
    obc_blk_parse_fdt(CONFIG_FDT_OBC_MMC_NODE, fdt_addr, pstAbi);

    /* 4# 解析设备树填充 obc-flash 分区信息 */
    obc_blk_parse_fdt(CONFIG_FDT_OBC_FLASH_NODE, fdt_addr, pstAbi);

    return 0;
}

int am62x_board_args_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    const char *boot_from_nfs = env_get("boot_from_nfs");

    /* 如果设置了 boot_from_nfs=1，跳过 bootargs 设置 */
    if (boot_from_nfs && (strcmp(boot_from_nfs, "1") == 0)) {
        printf("boot_from_nfs is set, skipping bootargs configuration\n");
        return 0;
    }

    /* 设置启动的bootargs参数，包含console、mmcblk */
    if (BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia) {
        if (pstAbi->stBoot.iPartType == OBC_SLOT_B)
            obc_bootargs_set(pstAbi, CONFIG_AM62X_SD_DEV_NAME,
                             CONFIG_AM62X_BOOTARGS_SD_B);
        else
            obc_bootargs_set(pstAbi, CONFIG_AM62X_SD_DEV_NAME,
                             CONFIG_AM62X_BOOTARGS_SD);
    } else {
        if (pstAbi->stBoot.iPartType == OBC_SLOT_B)
            obc_bootargs_set(pstAbi, CONFIG_AM62X_EMMC_DEV_NAME,
                             CONFIG_AM62X_BOOTARGS_EMMC_B);
        else
            obc_bootargs_set(pstAbi, CONFIG_AM62X_EMMC_DEV_NAME,
                             CONFIG_AM62X_BOOTARGS_EMMC);
    }

    return 0;
}

BOARD_CONFIG_TABLE_T g_am62x_board = {
    .board_hw_init          = am62x_board_hw_init,
    .board_env_init         = am62x_board_env_init,
    .board_fdt_init         = am62x_board_fdt_init,
    .board_args_init        = am62x_board_args_init,
};
