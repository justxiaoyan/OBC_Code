


#include <board_config_imx6ull.h>
#include <obc_blk.h>

#if CURRENT_UBOOT_VERSION >= UBOOT_VERSION(2024, 10, 0)
struct blk_desc *get_blk_dev(const char *ifname, int dev)
{
    return blk_get_devnum_by_type(blk_get_devtype(ifname), dev);
}
#endif
int imx6ull_board_hw_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    /* 启动设备编号获取 */
#if CURRENT_UBOOT_VERSION >= UBOOT_VERSION(2024, 10, 0)
    int bootdev = mmc_get_env_dev();
#else
    int bootdev = 1;//mmc_get_env_dev();
#endif

    /* 1# 启动设备内容获取 */
    pstAbi->stBoot.iMediaId = bootdev;
    if (CONFIG_IMX6ULL_DEV_SD == bootdev)
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_SD;
    else if (CONFIG_IMX6ULL_DEV_EMMC == bootdev)
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_EMMC;

    /* 2# EMMC控制器信息 */
    pstAbi->stBlk.iRdSize = 512;
#if CURRENT_UBOOT_VERSION >= UBOOT_VERSION(2024, 10, 0)
    pstAbi->stBlk.pstBlkDev = blk_get_dev("mmc", CONFIG_IMX6ULL_DEV_EMMC);
#else
    pstAbi->stBlk.pstBlkDev = mmc_get_blk_desc(find_mmc_device(bootdev));
#endif
    if (NULL == pstAbi->stBlk.pstBlkDev)
        printf("imx6ull_board_hw_init blkdev error\n");

    /* 3# 启动地址 */
    pstAbi->stBlk.def_fdt = 0x80000;
    pstAbi->stBoot.uiFdtAddr = CONFIG_IMX6ULL_HEX_FDT_ADDR;
    pstAbi->stBoot.uiImageAddr = CONFIG_IMX6ULL_HEX_KERNEL_ADDR;
    pstAbi->stBoot.uiTmpAddr = 0x84000000;

    return 0;
}

/* 环境变量初始化 */
int imx6ull_board_env_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    env_set("ipaddr", "10.10.0.221");
    env_set("netmask", "255.255.0.0");
    env_set("gatewayip", "10.10.0.101");
    env_set("serverip", "10.10.0.201");

    env_set("fdt_addr", CONFIG_IMX6ULL_STR_FDT_ADDR);
    env_set("loadaddr", CONFIG_IMX6ULL_STR_KERNEL_ADDR);

    env_set("mmcdev", "0");
    env_set("mmcpart", "1");

    env_set("bootdelay", "1");

    if (BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
    {
        env_set("loadimage", "fatload mmc ${sddev}:${sdpart} ${loadaddr} ${image}");
        env_set("sdboot", "run loadimage;bootz ${loadaddr} - ${fdt_addr}");
        env_set("bootcmd", "run sdboot");
    }
    else if (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia)
    {
        env_set("mmcboot", "bootk");
        env_set("bootcmd", "run mmcboot");
    }

    env_save();

    return 0;
}

int imx6ull_board_fdt_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    int iRet = 0;
    void *fdt_addr = (void *)CONFIG_IMX6ULL_HEX_FDT_ADDR;

    /* 1# 加载设备树,检查设备树是否有效 */
    iRet = obc_fdt_load_to_mem(pstAbi, CONFIG_IMX6ULL_STR_FDT_ADDR, CONFIG_IMX6ULL_FDT_NAME);
    if  (0 != iRet)
    {
        printf("fdt load to mem error\n");
        return -1;
    }

    if (fdt_check_header(fdt_addr) != 0)
    {
        printf("Error: Invalid device tree header\n");
        return -1;
    }

    /* 2# 解析设备树填充ability的dev part分区信息 */
    obc_blk_parse_fdt(CONFIG_IMX6ULL_EMMC_HOST_NAME, fdt_addr, pstAbi);

    return 0;
}

int imx6ull_board_args_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    /* 设置启动的bootargs参数，包含console、mmcblk */
    if (BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
        obc_bootargs_set(pstAbi, CONFIG_IMX6ULL_EMMC_DEV_NAME, CONFIG_IMX6ULL_BOOTARGS_SD);
    else
        obc_bootargs_set(pstAbi, CONFIG_IMX6ULL_EMMC_DEV_NAME, CONFIG_IMX6ULL_BOOTARGS_EMMC);

    return 0;
}

BOARD_CONFIG_TABLE_T g_imx6ull_board = {
    .board_hw_init          = imx6ull_board_hw_init,
    .board_env_init         = imx6ull_board_env_init,
    .board_fdt_init         = imx6ull_board_fdt_init,
    .board_args_init        = imx6ull_board_args_init,
};


