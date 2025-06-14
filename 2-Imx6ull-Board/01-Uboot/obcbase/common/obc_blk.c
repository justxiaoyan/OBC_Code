
#include <obc_blk.h>
#include <cpu_func.h>
#include <linux/delay.h>

void obc_blk_parse_partitions(BOARD_ABILITY_BLK_T *pstPart, const void *fdt, int node_offset)
{
    int subnode_offset;
    int label_len;
    const char *label;
    const fdt32_t *reg;
    int reg_len;
    const char *flag;
    int flag_len;
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo = pstPart->stParts;

    /* 遍历 partitions 节点中的所有子节点 */
    for (subnode_offset = fdt_first_subnode(fdt, node_offset);
         subnode_offset >= 0;
         subnode_offset = fdt_next_subnode(fdt, subnode_offset))
    {
        /* 获取 label 属性*/
        flag = fdt_getprop(fdt, subnode_offset, "bootable", &flag_len);
        if (flag)
        {
            continue;
        }

        /* 获取 label 属性*/
        label = fdt_getprop(fdt, subnode_offset, "label", &label_len);
        if (label)
        {
            memcpy(pblkinfo[pstPart->iPartCount].lable, label, label_len);
        }

        /* 获取 reg 属性 */
        reg = fdt_getprop(fdt, subnode_offset, "reg", &reg_len);
        if (reg)
        {
            pblkinfo[pstPart->iPartCount].addr = fdt32_to_cpu(reg[0]); /* 32-bit address */
            pblkinfo[pstPart->iPartCount].size = fdt32_to_cpu(reg[1]); /* 32-bit size */
        }

        printf("lable = %s, addr = 0x%x, size = 0x%x\n",
               pblkinfo[pstPart->iPartCount].lable,
               pblkinfo[pstPart->iPartCount].addr,
               pblkinfo[pstPart->iPartCount].size);

        pstPart->iPartCount++;
    }

    return ;
}

int obc_blk_find_parts_node(char *pHostName, const void *fdt, int node_offset)
{
    const char *compatible;
    int len;

    /* 检查当前节点是否是 &usdhc2 */
    compatible = fdt_getprop(fdt, node_offset, "obcpart", &len);
    if (compatible && (strstr(compatible, pHostName)))
    {
        return node_offset;
    }

    /* 递归查找子节点 */
    int subnode_offset;
    for (subnode_offset = fdt_first_subnode(fdt, node_offset);
         subnode_offset >= 0;
         subnode_offset = fdt_next_subnode(fdt, subnode_offset))
    {
        int found_node = obc_blk_find_parts_node(pHostName, fdt, subnode_offset);
        if (found_node >= 0)
        {
            return found_node;
        }
    }

    return -1;
}

void obc_blk_parse_fdt(char *pHostName,const void *fdt, BOARD_ABILITY_TABLE_T *pstAbi)
{
    int node_offset;
    int partitions_node;

    /* 从根节点开始查找 emmc host 节点 */
    node_offset = obc_blk_find_parts_node(pHostName, fdt, 0);
    if (node_offset < 0)
    {
        printf("Error: %s node not found\n", pHostName);
        return;
    }

    /* 打印找到的节点信息 */
    const char *node_name = fdt_get_name(fdt, node_offset, NULL);
    if (node_name)
    {
        printf("Found %s node: %s\n", pHostName, node_name);
    }
    else
    {
        printf("Error: Unable to get name of %s node\n", pHostName);
        return;
    }

    /* 查找 partitions 节点 */
    partitions_node = fdt_subnode_offset(fdt, node_offset, "partitions");
    if (partitions_node < 0)
    {
        printf("Error: partitions node not found\n");

        /* 打印 &usdhc2 节点的所有子节点，以确认 partitions 节点是否存在 */
        printf("Listing all subnodes of %s node:\n", pHostName);
        int subnode_offset;
        for (subnode_offset = fdt_first_subnode(fdt, node_offset);
             subnode_offset >= 0;
             subnode_offset = fdt_next_subnode(fdt, subnode_offset))
        {
            const char *subnode_name = fdt_get_name(fdt, subnode_offset, NULL);
            if (subnode_name)
            {
                printf("  Subnode: %s\n", subnode_name);
            }
            else
            {
                printf("  Error: Unable to get name of subnode\n");
            }
        }
        return;
    }

    /* 打印 partitions 节点信息 */
    const char *partitions_name = fdt_get_name(fdt, partitions_node, NULL);
    if (partitions_name)
    {
        printf("Found partitions node: %s\n", partitions_name);
    }
    else
    {
        printf("Error: Unable to get name of partitions node\n");
        return;
    }

    /* 解析 partitions 节点 */
    obc_blk_parse_partitions(&pstAbi->stPart, fdt, partitions_node);

    return;
}

/* 将十六进制地址和大小转换为字符串 */
char *hex_to_str(unsigned int value, char *buffer)
{
    sprintf(buffer, "0x%x", value);
    return buffer;
}

/* 将大小转换为更易读的格式（如KB、MB）*/
char *size_to_readable(unsigned int size, char *buffer)
{
    if (size % (1024 * 1024) == 0)
    {
        sprintf(buffer, "%dM", size / (1024 * 1024));
    }
    else if (size % 1024 == 0)
    {
        sprintf(buffer, "%dK", size / 1024);
    }
    else
    {
        sprintf(buffer, "0x%x", size);
    }
    return buffer;
}

/* 生成blkdevparts参数 */
void obc_bootargs_blkparts_set(BOARD_ABILITY_BLK_T *pstBlk, char *blkdevparts, const char *device)
{
    char temp[64];
    blkdevparts[0] = '\0';
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo = pstBlk->stParts;

    for (int i = 0; i < pstBlk->iPartCount; i++)
    {
        char size_str[16];
        char addr_str[16];
        size_to_readable(pblkinfo[i].size, size_str);
        hex_to_str(pblkinfo[i].addr, addr_str);

        sprintf(temp, "%s%s@%s(%s)", i > 0 ? ",": "", size_str, addr_str, pblkinfo[i].lable);
        strcat(blkdevparts, temp);
    }

    /* 添加设备名称 */
    sprintf(temp, "%s:", device);
    memmove(blkdevparts + strlen(device) + 1, blkdevparts, strlen(blkdevparts) + 1);
    memcpy(blkdevparts, temp, strlen(device) + 1);

    return ;
}

int obc_bootargs_set(BOARD_ABILITY_TABLE_T *pstAbi, char *pDevName, char *pConsole)
{
    char blkdevparts[512];
    char bootargs[1024];
    int len;

    obc_bootargs_blkparts_set(&pstAbi->stPart, blkdevparts, pDevName);
    printf("blkdevparts=%s\n", blkdevparts);

    // 将当前bootargs和blkdevparts拼接
    len = snprintf(bootargs, sizeof(bootargs), "setenv bootargs %s blkdevparts=%s", pConsole, blkdevparts);
    if (len >= sizeof(bootargs))
    {
        printf("Error: bootargs buffer overflow.\n");
        return CMD_RET_FAILURE;
    }

    run_command(bootargs, 0);
    printf("Updated bootargs: %s\n", bootargs);

    return 0;
}

int obc_fdt_load_to_mem(BOARD_ABILITY_TABLE_T *pstAbi, char *fdtaddr, char *pFdtName)
{
    char command[128] = {0};

    if (BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
    {
        memset(command, 0, sizeof(command));
        sprintf(command, "fatload mmc %d:%d %s %s",
                                        pstAbi->stBoot.iMediaId, 
                                        1,
                                        fdtaddr, 
                                        pFdtName);

        run_command(command, 0);
    }
    else if (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia)
    {

    }

    return 0;
}



