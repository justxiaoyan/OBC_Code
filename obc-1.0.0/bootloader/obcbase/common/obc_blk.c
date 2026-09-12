
#include <obc_blk.h>
#include <obc_pack.h>
#include <cpu_func.h>
#include <linux/delay.h>
#include <cmd_updatex.h>
#include <part.h>

/* 全局块设备描述符指针 */
struct blk_desc *gstBlkDev = NULL;



void obc_blk_parse_partitions(BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int *part_count, const void *fdt, int node_offset)
{
    int subnode_offset;
    int label_len;
    const char *label;
    const fdt32_t *reg;
    int reg_len;
    const char *flag;
    int flag_len;

    /* 遍历 partitions 节点中的所有子节点 */
    for (subnode_offset = fdt_first_subnode(fdt, node_offset);
         subnode_offset >= 0;
         subnode_offset = fdt_next_subnode(fdt, subnode_offset))
    {
        /* 获取 label 属性*/
        label = fdt_getprop(fdt, subnode_offset, "label", &label_len);
        if (label && *part_count < MAX_PART_NUM)
        {
            size_t copy_len = label_len < sizeof(pblkinfo[*part_count].lable) - 1 ?
                              label_len : sizeof(pblkinfo[*part_count].lable) - 1;
            memcpy(pblkinfo[*part_count].lable, label, copy_len);
            pblkinfo[*part_count].lable[copy_len] = '\0';
        }

        /* 获取 reg 属性 */
        reg = fdt_getprop(fdt, subnode_offset, "reg", &reg_len);
        if (reg && *part_count < MAX_PART_NUM && reg_len >= 2 * sizeof(fdt32_t))
        {
            pblkinfo[*part_count].addr = fdt32_to_cpu(reg[0]); /* 32-bit address */
            pblkinfo[*part_count].size = fdt32_to_cpu(reg[1]); /* 32-bit size */
        }

        /* 获取 bootable 标记并保存到 flag 字段 */
        flag = fdt_getprop(fdt, subnode_offset, "bootable", &flag_len);
        if (flag)
        {
            pblkinfo[*part_count].flag = 1;  /* 标记为 bootable */
        }
        else
        {
            pblkinfo[*part_count].flag = 0;  /* 非 bootable */
        }

#if 0
        printf("part[%d] lable = %s, addr = 0x%x, size = 0x%x, bootable = %d\n",
               *part_count + 1,
               pblkinfo[*part_count].lable,
               pblkinfo[*part_count].addr,
               pblkinfo[*part_count].size,
               pblkinfo[*part_count].flag);
#endif
        if (*part_count < MAX_PART_NUM)
            (*part_count)++;
        else
            break;
    }

    return ;
}

int obc_blk_find_parts_node(char *pHostName, const void *fdt, int node_offset)
{
    const char *obcpart;
    int len;

    /* 检查当前节点是否包含 obcpart 属性 */
    obcpart = fdt_getprop(fdt, node_offset, "obcpart", &len);
    if (obcpart && (strstr(obcpart, pHostName)))
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

void obc_blk_parse_fdt(char *pHostName, const void *fdt, BOARD_ABILITY_TABLE_T *pstAbi)
{
    int node_offset;
    int partitions_node;
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo;
    int *part_count;

    /* 从根节点开始查找 obcpart 节点 */
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

        /* 打印节点的所有子节点，以确认 partitions 节点是否存在 */
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

    /* 根据类型选择对应的分区表 */
    if (strstr(pHostName, CONFIG_FDT_OBC_MMC_NODE))
    {
        pblkinfo = pstAbi->stBlk.stParts_mmc;
        part_count = &pstAbi->stBlk.iPartCount_mmc;
    }
    else if (strstr(pHostName, CONFIG_FDT_OBC_FLASH_NODE))
    {
        pblkinfo = pstAbi->stBlk.stParts_flash;
        part_count = &pstAbi->stBlk.iPartCount_flash;
    }
    else
    {
        printf("Error: Unknown partition type %s\n", pHostName);
        return;
    }

    /* 解析 partitions 节点 */
    obc_blk_parse_partitions(pblkinfo, part_count, fdt, partitions_node);

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
    int first = 1;
    blkdevparts[0] = '\0';
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo;
    int part_count;

    /* 根据启动介质选择分区表 */
    if ((NULL != gstBlkDev)
        && (BOARD_ABILITY_DEV_FLASH == gstBlkDev->uclass_id))
    {
        /* Flash 启动，使用 flash 分区表 */
        pblkinfo = pstBlk->stParts_flash;
        part_count = pstBlk->iPartCount_flash;
    }
    else
    {
        /* MMC/SD 启动，使用 mmc 分区表 */
        pblkinfo = pstBlk->stParts_mmc;
        part_count = pstBlk->iPartCount_mmc;
    }

    for (int i = 0; i < part_count; i++)
    {
        /* 跳过 bootable 标记的分区 */
        if (pblkinfo[i].flag)
        {
            //printf("Skip bootable partition: %s\n", pblkinfo[i].lable);
            continue;
        }

        char size_str[16];
        char addr_str[16];
        size_to_readable(pblkinfo[i].size, size_str);
        hex_to_str(pblkinfo[i].addr, addr_str);

        sprintf(temp, "%s%s@%s(%s)", first ? "" : ",", size_str, addr_str, pblkinfo[i].lable);
        strcat(blkdevparts, temp);
        first = 0;
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

    // 将当前bootargs和blkdevparts拼接
    len = snprintf(bootargs, sizeof(bootargs), "setenv bootargs %s blkdevparts=%s", pConsole, blkdevparts);
    if (len >= sizeof(bootargs))
    {
        printf("Error: bootargs buffer overflow.\n");
        return CMD_RET_FAILURE;
    }

    run_command(bootargs, 0);
    //printf("Updated bootargs: %s\n", bootargs);

    return 0;
}

int obc_blk_find_part_by_name(BOARD_ABILITY_BLK_T *pstBlk, char *pName)
{
    int iIndex = 0;
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo;
    int part_count;

    /* 根据启动介质选择分区表 */
    if ((NULL != gstBlkDev)
        && (BOARD_ABILITY_DEV_FLASH == gstBlkDev->uclass_id))
    {
        /* Flash 启动，使用 flash 分区表 */
        pblkinfo = pstBlk->stParts_flash;
        part_count = pstBlk->iPartCount_flash;
    }
    else
    {
        /* MMC/SD 启动，使用 mmc 分区表 */
        pblkinfo = pstBlk->stParts_mmc;
        part_count = pstBlk->iPartCount_mmc;
    }

    for (iIndex = 0; iIndex < part_count; iIndex++)
    {
        if (0 == strcmp(pblkinfo[iIndex].lable, pName))
        {
            break;
        }
    }

    if (iIndex >= part_count)
    {
        printf("invalid part %s\n", pName);
        return -1;
    }

    return iIndex;
}

/* 通用的分区查找函数 */
int obc_blk_find_part_by_name_ex(BOARD_ABILITY_BLK_PARTS_T *pblkinfo, int part_count, char *pName)
{
    int iIndex = 0;

    for (iIndex = 0; iIndex < part_count; iIndex++)
    {
        if (0 == strcmp(pblkinfo[iIndex].lable, pName))
        {
            return iIndex;
        }
    }

    return -1;
}

int obc_blk_read_part_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *partname, unsigned int addr)
{
    ulong ulCnt = 0;
    ulong start, count;
    int iPartsIndex = 0;
    BOARD_ABILITY_BLK_PARTS_T *pstParts;
    OBC_PACK_HEAD_T *header;
    u8 *data_ptr;
    int ret;

    /* 找到名称对应的分区 */
    iPartsIndex = obc_blk_find_part_by_name(&pstAbi->stBlk, partname);
    if (iPartsIndex < 0)
    {
        printf("find part error %s\n", partname);
        return -1;
    }

    /* 根据启动介质选择分区表 */
    if ((NULL != gstBlkDev)
        && (BOARD_ABILITY_DEV_FLASH == gstBlkDev->uclass_id))
    {
        pstParts = pstAbi->stBlk.stParts_flash;
    }
    else
    {
        pstParts = pstAbi->stBlk.stParts_mmc;
    }

    /* 计算分区头部块偏移 */
    start = pstParts[iPartsIndex].addr / pstAbi->stBlk.iRdSize;

    /* 第一步：只读取包含头部的第一个块 */
    ulCnt = blk_dread(gstBlkDev, start, 1, (void *)addr);
    if (1 != ulCnt)
    {
        printf("read partition %s header error\n", partname);
        return -1;
    }

    /* 验证OBC打包头的魔数 */
    header = (OBC_PACK_HEAD_T *)addr;
    if (strncmp(header->magic, OBC_MAGIC, OBC_MAGIC_LEN - 1) != 0)
    {
        printf("Invalid OBC magic for partition %s\n", partname);
        return -1;
    }

    if (!header->file_size || header->file_size > pstParts[iPartsIndex].size - OBC_HEADER_SIZE)
    {
        printf("Invalid OBC payload size for partition %s\n", partname);
        return -1;
    }

    /* 计算读取的块数量 */
    count = header->file_size / pstAbi->stBlk.iRdSize;
    if (0 != (header->file_size % pstAbi->stBlk.iRdSize))
    {
        /* 不能被块大小整除就多写一个块 */
        count += 1;
    }

    data_ptr = (u8 *)addr + OBC_HEADER_SIZE;

    /* 如果还有剩余数据块需要读取 */
    ulCnt = blk_dread(gstBlkDev, start + 1, count, (void *)data_ptr);
    if (count != ulCnt)
    {
        printf("read partition %s remaining data error\n", partname);
        return -1;
    }

    /* 第三步：进行CRC16校验 */
    ret = obc_verify_pack_header(header, data_ptr);
    if (ret != 0)
    {
        printf("OBC pack header verification failed for partition %s, ret=%d\n", partname, ret);
        return -1;
    }

    /* 验证通过，将实际数据移动到目标地址（覆盖头部） */
    memmove((void *)addr, data_ptr, header->file_size);

    return 0;
}


int obc_fdt_load_to_mem(BOARD_ABILITY_TABLE_T *pstAbi, char *fdtaddr, char *pFdtName)
{
    int iRet = 0;
    int i = 0;
    const char *fdt_names[] = {"fdt0", "fdt1", "fdt2"};

    if ((BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
        || (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia))
    {
        /* 如果SPL阶段没有解析分区信息，则初始化默认的三个fdt分区 */
        if (0 == pstAbi->stBlk.iPartCount_mmc)
        {
            printf("Initializing default fdt partitions (SD)\n");
            pstAbi->stBlk.iPartCount_mmc = 3;

            /* fdt0 分区 */
            pstAbi->stBlk.stParts_mmc[0].addr = CONFIG_DEFAULT_AM62X_FDT0_DEVADDR;
            pstAbi->stBlk.stParts_mmc[0].size = CONFIG_DEFAULT_AM62X_FDT0_SIZE;
            memcpy(pstAbi->stBlk.stParts_mmc[0].lable, "fdt0", 5);

            /* fdt1 分区 */
            pstAbi->stBlk.stParts_mmc[1].addr = CONFIG_DEFAULT_AM62X_FDT1_DEVADDR;
            pstAbi->stBlk.stParts_mmc[1].size = CONFIG_DEFAULT_AM62X_FDT0_SIZE;
            memcpy(pstAbi->stBlk.stParts_mmc[1].lable, "fdt1", 5);

            /* fdt2 分区 */
            pstAbi->stBlk.stParts_mmc[2].addr = CONFIG_DEFAULT_AM62X_FDT2_DEVADDR;
            pstAbi->stBlk.stParts_mmc[2].size = CONFIG_DEFAULT_AM62X_FDT0_SIZE;
            memcpy(pstAbi->stBlk.stParts_mmc[2].lable, "fdt2", 5);
        }
        else
        {
            printf("Using existing partition info from SPL (SD), part_count=%d\n", pstAbi->stBlk.iPartCount_mmc);
        }

        /* 依次尝试从fdt0、fdt1、fdt2加载，任意一个成功即返回 */
        for (i = 0; i < 3; i++)
        {
            printf("Trying to load from %s...\n", fdt_names[i]);
            iRet = obc_blk_read_part_by_name(pstAbi, (char *)fdt_names[i], pstAbi->stBoot.uiFdtAddr);
            if (0 == iRet)
            {
                printf("Successfully loaded fdt from %s (mmc dev %d)\n", fdt_names[i], pstAbi->stBoot.iBootMedia);
                return 0;
            }
            printf("Failed to load from %s, trying next...\n", fdt_names[i]);
        }

        /* 所有分区都失败 */
        printf("get fdt failed from all partitions (mmc dev %d)\n", pstAbi->stBoot.iBootMedia);
        return -1;
    }
    else if (BOARD_ABILITY_DEV_FLASH == pstAbi->stBoot.iBootMedia)
    {
        /* todo */
    }

    return 0;
}


int obc_blk_write_part_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *pName, unsigned int fileaddr, int file_size)
{
    ulong ulCnt = 0;
    int iPartsIndex = 0;
    ulong start, count;
    BOARD_ABILITY_BLK_PARTS_T *pstParts;
    OBC_PACK_HEAD_T *header;
    u8 *write_addr;
    u32 write_size;
    int ret;

    /* 找到名称对应的分区 */
    iPartsIndex = obc_blk_find_part_by_name(&pstAbi->stBlk, pName);
    if (iPartsIndex < 0)
    {
        printf("find part error %s\n", pName);
        return -1;
    }

    /* 根据启动介质选择分区表 */
    if ((NULL != gstBlkDev )
        && (BOARD_ABILITY_DEV_FLASH == gstBlkDev->uclass_id))
    {
        pstParts = pstAbi->stBlk.stParts_flash;
    }
    else
    {
        pstParts = pstAbi->stBlk.stParts_mmc;
    }

    /* 验证OBC打包头 */
    header = (OBC_PACK_HEAD_T *)fileaddr;

    /* 检查是否有有效的OBC打包头 */
    if (strncmp(header->magic, OBC_MAGIC, OBC_MAGIC_LEN - 1) == 0)
    {
        /* 有OBC打包头，进行验证 */
        u8 *data_ptr = (u8 *)fileaddr + OBC_HEADER_SIZE;

        ret = obc_verify_pack_header(header, data_ptr);
        if (ret != 0)
        {
            printf("OBC pack header verification failed for upgrade file, ret=%d\n", ret);
            return -1;
        }

        /* 根据head_write_flag决定是否写入头部 */
        if (header->head_write_flag == 1)
        {
            /* 需要写入头部，写入整个文件（头部+数据） */
            write_addr = (u8 *)fileaddr;
            write_size = OBC_HEADER_SIZE + header->file_size;
            printf("Writing with OBC header (head_write_flag=1)\n");
        }
        else
        {
            /* 不需要写入头部，只写入实际数据 */
            write_addr = data_ptr;
            write_size = header->file_size;
            printf("Writing without OBC header (head_write_flag=0)\n");
        }
    }
    else
    {
        /* 没有OBC打包头，直接写入原始数据 */
        write_addr = (u8 *)fileaddr;
        write_size = file_size;
        printf("No OBC header found, writing raw data\n");
    }

    if (!write_size || write_size > pstParts[iPartsIndex].size)
    {
        printf("Data too large for partition %s\n", pName);
        return -1;
    }

    /* 计算写入的块数量 */
    start = pstParts[iPartsIndex].addr / pstAbi->stBlk.iRdSize;
    count = write_size / pstAbi->stBlk.iRdSize;
    if (0 != (write_size % pstAbi->stBlk.iRdSize))
    {
        /* 不能被块大小整除就多写一个块 */
        count += 1;
    }

    /* 写入数据到设备 */
    ulCnt = blk_dwrite(gstBlkDev, start, count, (void *)write_addr);
    if (count != ulCnt)
    {
        printf("write partition %s error\n", pName);
        return -1;
    }

    printf("Partition %s written successfully\n\n", pName);
    return 0;
}

/* 通用擦除接口 - MMC */
int obc_blk_erase_mmc_partition(BOARD_ABILITY_TABLE_T *pstAbi, char *partname)
{
    int part_index;
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo = pstAbi->stBlk.stParts_mmc;
    int part_count = pstAbi->stBlk.iPartCount_mmc;

    /* 查找分区 */
    part_index = obc_blk_find_part_by_name_ex(pblkinfo, part_count, partname);
    if (part_index < 0)
    {
        printf("Error: Partition '%s' not found in MMC partition table\n", partname);
        return -1;
    }

    /* 调用 MMC 特定的擦除函数 */
    return obc_blk_mmc_erase(pstAbi, pblkinfo, part_index);
}

/* 通用擦除接口 - Flash */
int obc_blk_erase_flash_partition(BOARD_ABILITY_TABLE_T *pstAbi, char *partname)
{
    int part_index;
    BOARD_ABILITY_BLK_PARTS_T *pblkinfo = pstAbi->stBlk.stParts_flash;
    int part_count = pstAbi->stBlk.iPartCount_flash;

    /* 查找分区 */
    part_index = obc_blk_find_part_by_name_ex(pblkinfo, part_count, partname);
    if (part_index < 0)
    {
        printf("Error: Partition '%s' not found in Flash partition table\n", partname);
        return -1;
    }

    /* 调用 Flash 特定的擦除函数 */
    return obc_blk_flash_erase(pstAbi, pblkinfo, part_index);
}
