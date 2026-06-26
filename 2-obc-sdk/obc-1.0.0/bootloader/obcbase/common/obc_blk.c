
#include <obc_blk.h>
#include <cpu_func.h>
#include <linux/delay.h>
#include <cmd_updatex.h>
#include <part.h>



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
#if 1
        printf("part[%d] lable = %s, addr = 0x%x, size = 0x%x\n",
               pstPart->iPartCount,
               pblkinfo[pstPart->iPartCount].lable,
               pblkinfo[pstPart->iPartCount].addr,
               pblkinfo[pstPart->iPartCount].size);
#endif
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

#if 1
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
#endif

    /* 如果是emmc设备启动，需要在解析设备树前清空默认值 */
    if (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia)
    {
        pstAbi->stBlk.iPartCount = 0;
        memset((void *)&pstAbi->stBlk.stParts[0], 0, sizeof(BOARD_ABILITY_BLK_PARTS_T));
    }

    /* 解析 partitions 节点 */
    obc_blk_parse_partitions(&pstAbi->stBlk, fdt, partitions_node);

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

    obc_bootargs_blkparts_set(&pstAbi->stBlk, blkdevparts, pDevName);
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

int obc_blk_find_part_by_name(BOARD_ABILITY_BLK_T *pstBlk, char *pName)
{
    int iIndex = 0;
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo = pstBlk->stParts;

    for (iIndex = 0; iIndex < pstBlk->iPartCount ;iIndex++)
    {
        printf("lable = %s, name = %s\n", pblkinfo[iIndex].lable, pName);
        if (0 == strcmp(pblkinfo[iIndex].lable, pName))
        {
            break;
        }
    }

    if (iIndex >= pstBlk->iPartCount)
    {
        printf("invaild part %s\n", pName);
        return -1;
    }

    return iIndex;
}


int obc_blk_read_part_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *partname, unsigned int addr)
{
    ulong ulCnt = 0;
    OBC_PACK_HEAD_T *pstHead;
    ulong start, count;
    int iPartsIndex = 0;
    BOARD_ABILITY_BLK_PARTS_T *pstParts = pstAbi->stBlk.stParts;

    /* 找到名称对应的分区 */
    iPartsIndex = obc_blk_find_part_by_name(&pstAbi->stBlk, partname);
    if (iPartsIndex < 0)
    {
        printf("find part error %s\n", partname);
        return -1;
    }

    start = pstParts[iPartsIndex].addr / pstAbi->stBlk.iRdSize;

    /* 1# 读取一个块，获取数据头 */
    ulCnt = blk_dread(pstAbi->stBlk.pstBlkDev, start, 1, (void *)pstAbi->stBoot.uiTmpAddr);
    if (1 != ulCnt)
    {
        printf("get haed info error\n");
        return -1;
    }

    pstHead = (OBC_PACK_HEAD_T *)pstAbi->stBoot.uiTmpAddr;

    /* 计算读取的块数量 */
    count = pstHead->file_size / pstAbi->stBlk.iRdSize;
    if (0 != (pstHead->file_size % pstAbi->stBlk.iRdSize))
    {
        /* 不能被块大小整除就多读一个块 */
        count += 1;
    }

    /* 加载真实数据到内存 */
    ulCnt = blk_dread(pstAbi->stBlk.pstBlkDev, start + 1, count, (void *)addr);
    if (count != ulCnt)
    {
        printf("get partname info error\n");
        return -1;
    }

    return 0;
}


int obc_fdt_load_to_mem(BOARD_ABILITY_TABLE_T *pstAbi, char *fdtaddr, char *pFdtName)
{
    int iRet = 0;
    char command[128] = {0};

    if (BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
    {
        /* select src dev boot0 */
        sprintf(command, "mmc dev 0");
        run_command(command, 0);

        memset(command, 0, sizeof(command));
        sprintf(command, "fatload mmc %d:%d %s %s",
                                        0, 
                                        1,
                                        fdtaddr, 
                                        pFdtName);

        run_command(command, 0);
    }
    else if (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia)
    {
        /* emmc分区第一次启动拿不到分区的，赋值一个默认值 */
        if (0 == pstAbi->stBlk.iPartCount)
        {
            pstAbi->stBlk.iPartCount = 1;
            pstAbi->stBlk.stParts[0].addr = pstAbi->stBlk.def_fdt;
            memcpy(pstAbi->stBlk.stParts[0].lable, "fdt0", 4);
        }

        /* 从分区获取文件加载到内存 */
        iRet=  obc_blk_read_part_by_name(pstAbi, "fdt0", pstAbi->stBoot.uiFdtAddr);
        if (0 != iRet)
        {
            printf("get fdt failed %d\n", iRet);
            return -1;
        }
    }

    return 0;
}


int obc_blk_write_part_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *pName, unsigned int fileaddr, int file_size)
{
    ulong ulCnt = 0;
    int iPartsIndex = 0;
    ulong start, count;
    BOARD_ABILITY_BLK_PARTS_T *pstParts = pstAbi->stBlk.stParts;

    /* 找到名称对应的分区 */
    iPartsIndex = obc_blk_find_part_by_name(&pstAbi->stBlk, pName);
    if (iPartsIndex < 0)
    {
        printf("find part error %s\n", pName);
        return -1;
    }

    /* 计算写入的块数量 */
    start = pstParts[iPartsIndex].addr / pstAbi->stBlk.iRdSize;
    count = file_size / pstAbi->stBlk.iRdSize;
    if (0 != (file_size % pstAbi->stBlk.iRdSize))
    {
        /* 不能被块大小整除就多读一个块 */
        count += 1;
    }

    /* 加载数据到设备 */
    ulCnt = blk_dwrite(pstAbi->stBlk.pstBlkDev, start, count, (void *)fileaddr);
    if (count != ulCnt)
    {
        printf("write part info error\n");
        return -1;
    }

    return 0;
}



