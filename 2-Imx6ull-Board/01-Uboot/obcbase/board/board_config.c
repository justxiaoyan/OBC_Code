

#include <board_config_imx6ull.h>
#include <board_config.h>



/* 板级BSP管理结构体 */
typedef struct BOARD_ABILITY_MANAGER
{
    BOARD_ABILITY_TABLE_T       stAbility;                  /* 能力集 */
    BOARD_CONFIG_TABLE_T        *pstBoard;                   /* 板级相关操作结构体 */
}BOARD_ABILITY_MANAGER_T;

BOARD_ABILITY_MANAGER_T g_obc_ability_manager;


BOARD_ABILITY_TABLE_T *obc_ability_get(void)
{
    return &g_obc_ability_manager.stAbility;
}

int obc_board_hw_init(void)
{
    if ((NULL == g_obc_ability_manager.pstBoard)
        || (NULL == g_obc_ability_manager.pstBoard->board_hw_init))
    {
        printf("board_hw_init is NULL\n");
        return -1;
    }

    return g_obc_ability_manager.pstBoard->board_hw_init(&g_obc_ability_manager.stAbility);
}

int obc_board_env_init(void)
{
    if ((NULL == g_obc_ability_manager.pstBoard)
        || (NULL == g_obc_ability_manager.pstBoard->board_env_init))
    {
        printf("board_env_init is NULL\n");
        return -1;
    }

    return g_obc_ability_manager.pstBoard->board_env_init(&g_obc_ability_manager.stAbility);
}

int obc_board_fdt_init(void)
{
    if ((NULL == g_obc_ability_manager.pstBoard)
        || (NULL == g_obc_ability_manager.pstBoard->board_fdt_init))
    {
        printf("board_fdt_init is NULL\n");
        return -1;
    }

    return g_obc_ability_manager.pstBoard->board_fdt_init(&g_obc_ability_manager.stAbility);
}

int obc_board_args_init(void)
{
    if ((NULL == g_obc_ability_manager.pstBoard)
        || (NULL == g_obc_ability_manager.pstBoard->board_args_init))
    {
        printf("board_args_init is NULL\n");
        return -1;
    }

    return g_obc_ability_manager.pstBoard->board_args_init(&g_obc_ability_manager.stAbility);
}

int obc_board_init(void)
{
    memset(&g_obc_ability_manager, 0x00, sizeof(g_obc_ability_manager));

#if defined(CONFIG_BOARD_CONFIG_IMX6ULL)
    g_obc_ability_manager.pstBoard = &g_imx6ull_board;
#endif

    /* 1# 板级硬件初始化 */
    (void)obc_board_hw_init();

    /* 2# 环境变量设置*/
    (void)obc_board_env_init();

    /* 3# 设备树解析 */
    (void)obc_board_fdt_init();

    /* 3# 启动参数配置解析 */
    (void)obc_board_args_init();

    return 0;
}














