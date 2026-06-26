


#include <obc_blk.h>
#include <board_config.h>




static int do_bootk(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    BOARD_ABILITY_TABLE_T *pstAbi = NULL;
    int iRet = 0;
    char command[128] = {0};

    pstAbi = obc_ability_get();

    /* 加载内核 */
    iRet = obc_blk_read_part_by_name(pstAbi, "kernel0", pstAbi->stBoot.uiImageAddr);
    if (0 != iRet)
    {
        printf("load kernel image error\n");
        return -1;
    }

    /* bootz启动内核 */
    sprintf(command, "bootz 0x%x - 0x%x", pstAbi->stBoot.uiImageAddr, pstAbi->stBoot.uiFdtAddr);
    run_command(command, 0);

    return 0;
}


U_BOOT_CMD(
	bootk, 1, 0, do_bootk,
	"load kernel run",
	"- updatex dev <emmc/sd/nor>\n"
	"- updatex src <sd/tftp>\n"
	"- updatex up <uboot/kernel/all>\n"
);





