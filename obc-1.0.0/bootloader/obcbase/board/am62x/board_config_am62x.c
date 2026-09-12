


#include <board_config_am62x.h>
#include <obc_blk.h>


int am62x_board_hw_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    int bootdev = BOARD_ABILITY_DEV_SD;

    /* 1# 启动设备内容获取 */
    pstAbi->stBoot.iMediaId = bootdev;
    if (CONFIG_AM62X_DEV_SD == bootdev)
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_SD;
    else if (CONFIG_AM62X_DEV_EMMC == bootdev)
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_EMMC;

    /* 2# EMMC控制器信息 */
    pstAbi->stBlk.iRdSize = 512;
    gstBlkDev = mmc_get_blk_desc(find_mmc_device(bootdev));

    if (NULL == gstBlkDev)
        printf("am62x_board_hw_init blkdev error\n");

    /* 3# 启动地址 */
    pstAbi->stBlk.def_fdt = CONFIG_DEFAULT_AM62X_FDT0_DEVADDR;
    pstAbi->stBoot.uiFdtAddr = CONFIG_AM62X_HEX_FDT_ADDR;
    pstAbi->stBoot.uiImageAddr = CONFIG_AM62X_HEX_KERNEL_ADDR;
    pstAbi->stBoot.uiTmpAddr = 0x84000000;

    return 0;
}

/* 环境变量初始化 */
int am62x_board_env_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    if (NULL == env_get("bootdelay"))
    {
        env_set("ipaddr", "192.168.18.140");
        env_set("netmask", "255.255.0.0");
        env_set("gatewayip", "192.168.18.1");
        env_set("serverip", "192.168.18.136");

        env_set("fdt_addr", CONFIG_AM62X_STR_FDT_ADDR);
        env_set("loadaddr", CONFIG_AM62X_STR_KERNEL_ADDR);

        env_set("mmcdev", "1");
        env_set("mmcpart", "1");

        env_set("bootdelay", "1");

        if ((BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
            || (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia))
        {
            env_set("mmcboot", "bootk");
            env_set("bootcmd", "run mmcboot");
        }

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

        memcpy(pstAbi->stBlk.stParts_mmc, spl_shared_ability->stBlk.stParts_mmc,
               sizeof(pstAbi->stBlk.stParts_mmc));
        memcpy(pstAbi->stBlk.stParts_flash, spl_shared_ability->stBlk.stParts_flash,
               sizeof(pstAbi->stBlk.stParts_flash));

        /* 仍然需要加载设备树到内存用于内核启动 */
        iRet = obc_fdt_load_to_mem(pstAbi, CONFIG_AM62X_STR_FDT_ADDR, CONFIG_AM62X_FDT_NAME);
        if (0 != iRet)
        {
            printf("U-Boot: fdt load to mem error\n");
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
    if (BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
        obc_bootargs_set(pstAbi, CONFIG_AM62X_SD_DEV_NAME, CONFIG_AM62X_BOOTARGS_SD);
    else
        obc_bootargs_set(pstAbi, CONFIG_AM62X_EMMC_DEV_NAME, CONFIG_AM62X_BOOTARGS_EMMC);

    return 0;
}

BOARD_CONFIG_TABLE_T g_am62x_board = {
    .board_hw_init          = am62x_board_hw_init,
    .board_env_init         = am62x_board_env_init,
    .board_fdt_init         = am62x_board_fdt_init,
    .board_args_init        = am62x_board_args_init,
};

