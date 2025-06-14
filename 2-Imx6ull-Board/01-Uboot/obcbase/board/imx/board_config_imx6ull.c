


#include <board_config_imx6ull.h>
#include <obc_blk.h>


int imx6ull_board_hw_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    /* 启动设备编号获取 */
    int bootdev = mmc_get_env_dev();

    pstAbi->stBoot.iMediaId = bootdev;

    if (CONFIG_IMX6ULL_DEV_SD == bootdev)
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_SD;
    else
        pstAbi->stBoot.iBootMedia = BOARD_ABILITY_DEV_EMMC;

    return 0;
}

/* 环境变量初始化 */
int imx6ull_board_env_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    env_set("ipaddr", "10.10.0.221");
    env_set("netmask", "255.255.0.0");
    env_set("gatewayip", "10.10.0.101");

    env_set("fdt_addr", CONFIG_IMX6ULL_FDT_ADDR);
    env_set("loadaddr", CONFIG_IMX6ULL_KERNEL_ADDR);

    env_set("sddev", "0");
    env_set("sdpart", "1");

    env_set("loadimage", "fatload mmc ${sddev}:${sdpart} ${loadaddr} ${image}");
    env_set("sdboot", "run loadimage;bootz ${loadaddr} - ${fdt_addr}");

    if (BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
        env_set("bootcmd", "run sdboot");
    else
        env_set("bootcmd", "run mmcboot");

    return 0;
}

int imx6ull_board_fdt_init(BOARD_ABILITY_TABLE_T *pstAbi)
{
    int iRet = 0;
    void *fdt_addr = (void *)simple_strtoul(CONFIG_IMX6ULL_FDT_ADDR, NULL, 16);

    /* 1# 加载设备树,检查设备树是否有效 */
    iRet = obc_fdt_load_to_mem(pstAbi, CONFIG_IMX6ULL_FDT_ADDR, CONFIG_IMX6ULL_FDT_NAME);
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
    obc_bootargs_set(pstAbi, CONFIG_IMX6ULL_EMMC_DEV_NAME, CONFIG_IMX6ULL_BOOTARGS);

    return 0;
}

BOARD_CONFIG_TABLE_T g_imx6ull_board = {
    .board_hw_init          = imx6ull_board_hw_init,
    .board_env_init         = imx6ull_board_env_init,
    .board_fdt_init         = imx6ull_board_fdt_init,
    .board_args_init        = imx6ull_board_args_init,
};


