


#include <obc_blk.h>
#include <board_config.h>
#include <board_config_am62x.h>
#include <gzip.h>

/* 压缩内核临时加载地址 */
#define KERNEL_COMP_LOAD_ADDR    0x90000000

static int do_bootk(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    BOARD_ABILITY_TABLE_T *pstAbi = NULL;
    int iRet = 0;
    char command[256] = {0};
    char kernel_name[32] = {0};
    unsigned char *comp_addr;
    unsigned long target_addr;
    int slot;

    pstAbi = obc_ability_get();

    printf("bootk: kernel=0x%x, fdt=0x%x\n", pstAbi->stBoot.uiImageAddr, pstAbi->stBoot.uiFdtAddr);

    /* kernel 与 TEE/U-Boot 使用同一槽位，不允许跨槽位回退。 */
    slot = pstAbi->stBoot.iPartType <= OBC_SLOT_B ? pstAbi->stBoot.iPartType : OBC_SLOT_A;
    sprintf(kernel_name, "kernel%d", slot);
    iRet = obc_blk_read_part_by_name(pstAbi, kernel_name, KERNEL_COMP_LOAD_ADDR);
    if (0 != iRet) {
        printf("Failed to load %s; switching to slot %c for next boot\n",
               kernel_name, 'A' + (slot == OBC_SLOT_A ? OBC_SLOT_B : OBC_SLOT_A));
        obc_slot_switch(slot);
    }

    /* 如果所有 kernel 分区都加载失败 */
    if (0 != iRet)
    {
        printf("Failed to load kernel slot %c\n", 'A' + slot);
        return -1;
    }

    comp_addr = (unsigned char *)KERNEL_COMP_LOAD_ADDR;
    target_addr = pstAbi->stBoot.uiImageAddr;

    /* 检查是否为 gzip 压缩内核 (magic: 0x1f 0x8b) */
    if (comp_addr[0] == 0x1f && comp_addr[1] == 0x8b)
    {
        printf("Compressed kernel (gzip) detected, decompressing to 0x%lx...\n", target_addr);

        /* 使用 U-Boot 的 unzip 命令解压 */
        sprintf(command, "unzip 0x%x 0x%lx", KERNEL_COMP_LOAD_ADDR, target_addr);
        iRet = run_command(command, 0);

        if (iRet != 0)
        {
            printf("Failed to decompress kernel; switching slot for next boot\n");
            obc_slot_switch(slot);
            return -1;
        }

        printf("Kernel decompressed successfully\n");
    }
    else
    {
        printf("Uncompressed kernel detected, copying to 0x%lx...\n", target_addr);

        /* 未压缩内核，直接拷贝 */
        sprintf(command, "cp.b 0x%x 0x%lx 0x2000000", KERNEL_COMP_LOAD_ADDR, target_addr);
        iRet = run_command(command, 0);
        if (iRet != 0) {
            printf("Failed to copy kernel; switching slot for next boot\n");
            obc_slot_switch(slot);
            return -1;
        }
    }

    /* booti 启动内核 */
    sprintf(command, "booti 0x%lx - 0x%x", target_addr, pstAbi->stBoot.uiFdtAddr);
    iRet = run_command(command, 0);
    if (iRet != 0) {
        printf("Kernel boot failed; switching to slot %c for next boot\n",
               'A' + (slot == OBC_SLOT_A ? OBC_SLOT_B : OBC_SLOT_A));
        obc_slot_switch(slot);
        return -1;
    }

    return 0;
}


U_BOOT_CMD(
	bootk, 1, 0, do_bootk,
	"load kernel run",
	"- updatex -d <sd|emmc>\n"
	"- updatex -s <sd|tftp>\n"
	"- updatex up <loader|fdt|teeos|kernel|rootfs|all>\n"
);
