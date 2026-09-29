
#include <obc_blk.h>
#include <obc_pack.h>
#include <cpu_func.h>
#include <linux/delay.h>
#include <cmd_updatex.h>
#include <image-sparse.h>
#include <part.h>
#include <sparse_format.h>
#if defined(CONFIG_BOARD_CONFIG_AM62X)
#include <board_config_am62x.h>
#elif defined(CONFIG_BOARD_CONFIG_IMX6ULL)
#include <board_config_imx6ull.h>
#endif

/* 全局块设备描述符指针 */
struct blk_desc *gstBlkDev = NULL;

#if defined(CONFIG_BOARD_CONFIG_AM62X)
int obc_slot_read(unsigned char *slot)
{
    unsigned char block[512];
    ulong sector;

    if (!slot || !gstBlkDev)
        return -1;

    sector = CONFIG_AM62X_SYSFLAG_DEVADDR / gstBlkDev->blksz;
    if (blk_dread(gstBlkDev, sector, 1, block) != 1)
        return -1;

    *slot = (block[0] == OBC_SLOT_B) ? OBC_SLOT_B : OBC_SLOT_A;
    return 0;
}

int obc_slot_write(unsigned char slot)
{
    unsigned char block[512];
    ulong sector;

    if (!gstBlkDev || slot > OBC_SLOT_B)
        return -1;

    sector = CONFIG_AM62X_SYSFLAG_DEVADDR / gstBlkDev->blksz;
    if (blk_dread(gstBlkDev, sector, 1, block) != 1)
        return -1;

    block[0] = slot;
    return blk_dwrite(gstBlkDev, sector, 1, block) == 1 ? 0 : -1;
}

int obc_slot_switch(unsigned char slot)
{
    unsigned char next = slot == OBC_SLOT_A ? OBC_SLOT_B : OBC_SLOT_A;

    printf("OBC: switching boot slot %c -> %c\n", 'A' + slot, 'A' + next);
    return obc_slot_write(next);
}
#endif



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
	const char *boot_media;
	int len;

	obc_bootargs_blkparts_set(&pstAbi->stBlk, blkdevparts, pDevName);
	boot_media = pstAbi->stBoot.iBootMedia == BOARD_ABILITY_DEV_SD ?
		"sd" : pstAbi->stBoot.iBootMedia == BOARD_ABILITY_DEV_EMMC ?
		"emmc" : "unknown";

	/* Publish the boot source in the same command line Linux already receives. */
	len = snprintf(bootargs, sizeof(bootargs),
		       "setenv bootargs %s blkdevparts=%s obcboot.part=%u obcboot.media=%s",
		       pConsole, blkdevparts, pstAbi->stBoot.iPartType, boot_media);
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
#if defined(CONFIG_BOARD_CONFIG_AM62X)
    int iRet = 0;
    int i = 0;
    const char *fdt_names[] = {"fdt0", "fdt1"};

    if ((BOARD_ABILITY_DEV_SD == pstAbi->stBoot.iBootMedia)
        || (BOARD_ABILITY_DEV_EMMC == pstAbi->stBoot.iBootMedia))
    {
        /* 如果SPL阶段没有解析分区信息，则初始化默认的两个fdt分区 */
        if (0 == pstAbi->stBlk.iPartCount_mmc)
        {
            printf("Initializing default FDT partitions for MMC device %d\n",
                   pstAbi->stBoot.iMediaId);
            pstAbi->stBlk.iPartCount_mmc = 2;

            /* fdt0 分区 */
            pstAbi->stBlk.stParts_mmc[0].addr = CONFIG_DEFAULT_AM62X_FDT0_DEVADDR;
            pstAbi->stBlk.stParts_mmc[0].size = CONFIG_DEFAULT_AM62X_FDT0_SIZE;
            memcpy(pstAbi->stBlk.stParts_mmc[0].lable, "fdt0", 5);

            /* fdt1 分区 */
            pstAbi->stBlk.stParts_mmc[1].addr = CONFIG_DEFAULT_AM62X_FDT1_DEVADDR;
            pstAbi->stBlk.stParts_mmc[1].size = CONFIG_DEFAULT_AM62X_FDT0_SIZE;
            memcpy(pstAbi->stBlk.stParts_mmc[1].lable, "fdt1", 5);

        }
        else
        {
            printf("Using existing partition info from SPL for MMC device %d, part_count=%d\n",
                   pstAbi->stBoot.iMediaId, pstAbi->stBlk.iPartCount_mmc);
        }

        /* 只加载当前槽位的 FDT，避免不同槽位的组件混用。 */
        i = pstAbi->stBoot.iPartType <= OBC_SLOT_B ? pstAbi->stBoot.iPartType : OBC_SLOT_A;
        for (; i < 2 && i == pstAbi->stBoot.iPartType; i++)
        {
            printf("Trying to load from %s...\n", fdt_names[i]);
            iRet = obc_blk_read_part_by_name(pstAbi, (char *)fdt_names[i], pstAbi->stBoot.uiFdtAddr);
            if (0 == iRet)
            {
                printf("Successfully loaded fdt from %s (mmc dev %d)\n", fdt_names[i], pstAbi->stBoot.iBootMedia);
                return 0;
            }
            printf("Failed to load from selected slot %s\n", fdt_names[i]);
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
#else
    return obc_blk_read_part_by_name(pstAbi, pFdtName, pstAbi->stBoot.uiFdtAddr);
#endif
}


struct obc_write_image {
    u8 *payload;
    u32 payload_size;
    u8 *write_data;
    u32 write_size;
    bool has_header;
    u16 head_write_flag;
};

static int obc_prepare_write_image(unsigned int fileaddr, int file_size,
                                   struct obc_write_image *image)
{
    OBC_PACK_HEAD_T *header = (OBC_PACK_HEAD_T *)fileaddr;
    u8 *data = (u8 *)fileaddr;

    if (!image || file_size <= 0)
        return -1;

    memset(image, 0, sizeof(*image));
    if (file_size < OBC_MAGIC_LEN - 1 ||
        strncmp(header->magic, OBC_MAGIC, OBC_MAGIC_LEN - 1) != 0)
    {
        image->payload = data;
        image->payload_size = file_size;
        image->write_data = data;
        image->write_size = file_size;
        printf("No OBC header found, using raw data\n");
        return 0;
    }

    if (file_size < OBC_HEADER_SIZE)
    {
        printf("Truncated OBC header: downloaded %d bytes\n", file_size);
        return -1;
    }
    if (!header->file_size || header->file_size != file_size - OBC_HEADER_SIZE)
    {
        printf("Invalid OBC payload length: header=%u downloaded=%d\n",
               header->file_size, file_size - OBC_HEADER_SIZE);
        return -1;
    }
    if (header->head_write_flag > 1)
    {
        printf("Invalid OBC head_write_flag: %u\n", header->head_write_flag);
        return -1;
    }

    image->payload = data + OBC_HEADER_SIZE;
    image->payload_size = header->file_size;
    if (obc_verify_pack_header(header, image->payload) != 0)
    {
        printf("OBC pack header verification failed for upgrade file\n");
        return -1;
    }

    image->has_header = true;
    image->head_write_flag = header->head_write_flag;
    if (header->head_write_flag)
    {
        image->write_data = data;
        image->write_size = file_size;
        printf("Writing with OBC header (head_write_flag=1)\n");
    }
    else
    {
        image->write_data = image->payload;
        image->write_size = image->payload_size;
        printf("Writing without OBC header (head_write_flag=0)\n");
    }

    return 0;
}

int obc_blk_write_part_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *pName, unsigned int fileaddr, int file_size)
{
    ulong ulCnt = 0;
    int iPartsIndex = 0;
    ulong start, count;
    BOARD_ABILITY_BLK_PARTS_T *pstParts;
    struct obc_write_image image;

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

    if (obc_prepare_write_image(fileaddr, file_size, &image) != 0)
        return -1;

    if (image.write_size > pstParts[iPartsIndex].size)
    {
        printf("Data too large for partition %s\n", pName);
        return -1;
    }

    /* 计算写入的块数量 */
    start = pstParts[iPartsIndex].addr / pstAbi->stBlk.iRdSize;
    count = image.write_size / pstAbi->stBlk.iRdSize;
    if (0 != (image.write_size % pstAbi->stBlk.iRdSize))
    {
        /* 不能被块大小整除就多写一个块 */
        count += 1;
    }

    /* 写入数据到设备 */
    ulCnt = blk_dwrite(gstBlkDev, start, count, image.write_data);
    if (count != ulCnt)
    {
        printf("write partition %s error\n", pName);
        return -1;
    }

    printf("Partition %s written successfully\n\n", pName);
    return 0;
}

#if CONFIG_IS_ENABLED(IMAGE_SPARSE)
static int obc_validate_sparse(const u8 *data, u32 data_size, u32 storage_blksz,
                               u64 partition_size)
{
    const sparse_header_t *header;
    u64 expanded_size;
    u64 total_blocks = 0;
    u32 offset;
    u32 i;
    u32 block_size;
    u32 total_chunks;
    u32 expected_blocks;
    u16 file_header_size;
    u16 chunk_header_size;

    if (data_size < sizeof(*header))
        return -1;

    header = (const sparse_header_t *)data;
    file_header_size = le16_to_cpu(header->file_hdr_sz);
    chunk_header_size = le16_to_cpu(header->chunk_hdr_sz);
    block_size = le32_to_cpu(header->blk_sz);
    expected_blocks = le32_to_cpu(header->total_blks);
    total_chunks = le32_to_cpu(header->total_chunks);

    if (le32_to_cpu(header->magic) != SPARSE_HEADER_MAGIC ||
        le16_to_cpu(header->major_version) != 1 ||
        file_header_size != sizeof(*header) || file_header_size > data_size ||
        chunk_header_size != sizeof(chunk_header_t) || !block_size ||
        block_size % 4 || block_size % storage_blksz || !expected_blocks)
    {
        printf("Invalid sparse image header\n");
        return -1;
    }

    expanded_size = (u64)block_size * expected_blocks;
    if (expanded_size / block_size != expected_blocks ||
        expanded_size > partition_size)
    {
        printf("Sparse image expands beyond partition (%llu > %llu)\n",
               expanded_size, partition_size);
        return -1;
    }

    offset = file_header_size;
    for (i = 0; i < total_chunks; i++)
    {
        const chunk_header_t *chunk;
        u32 chunk_blocks;
        u32 chunk_size;
        u64 expected_size;

        if (offset > data_size || chunk_header_size > data_size - offset)
        {
            printf("Sparse chunk %u header is truncated\n", i);
            return -1;
        }

        chunk = (const chunk_header_t *)(data + offset);
        chunk_blocks = le32_to_cpu(chunk->chunk_sz);
        chunk_size = le32_to_cpu(chunk->total_sz);
        if (chunk_size < chunk_header_size || chunk_size > data_size - offset)
        {
            printf("Sparse chunk %u exceeds downloaded image\n", i);
            return -1;
        }

        switch (le16_to_cpu(chunk->chunk_type))
        {
        case CHUNK_TYPE_RAW:
            expected_size = (u64)chunk_header_size +
                            (u64)chunk_blocks * block_size;
            break;
        case CHUNK_TYPE_FILL:
            expected_size = chunk_header_size + sizeof(u32);
            break;
        case CHUNK_TYPE_DONT_CARE:
            expected_size = chunk_header_size;
            break;
        case CHUNK_TYPE_CRC32:
            if (chunk_blocks != 0)
            {
                printf("Sparse CRC32 chunk %u has output blocks\n", i);
                return -1;
            }
            expected_size = chunk_header_size + sizeof(u32);
            break;
        default:
            printf("Sparse chunk %u has unknown type 0x%x\n", i,
                   le16_to_cpu(chunk->chunk_type));
            return -1;
        }

        if (expected_size != chunk_size ||
            total_blocks + chunk_blocks < total_blocks ||
            total_blocks + chunk_blocks > expected_blocks)
        {
            printf("Sparse chunk %u has invalid size or block count\n", i);
            return -1;
        }
        total_blocks += chunk_blocks;
        offset += chunk_size;
    }

    if (total_blocks != expected_blocks || offset != data_size)
    {
        printf("Sparse image totals do not match its header\n");
        return -1;
    }

    return 0;
}

static lbaint_t obc_sparse_write(struct sparse_storage *info, lbaint_t block,
                                 lbaint_t count, const void *buffer)
{
    return blk_dwrite(info->priv, block, count, buffer);
}

static lbaint_t obc_sparse_reserve(struct sparse_storage *info, lbaint_t block,
                                   lbaint_t count)
{
    return count;
}

int obc_blk_write_rootfs_by_name(BOARD_ABILITY_TABLE_T *pstAbi, char *pName,
                                 unsigned int fileaddr, int file_size)
{
    struct obc_write_image image;
    struct sparse_storage sparse;
    BOARD_ABILITY_BLK_PARTS_T *parts;
    int part_index;
    u32 block_size;
    u16 ext_magic;
    lbaint_t start;
    lbaint_t count;

    if (!gstBlkDev || !gstBlkDev->blksz)
        return -1;

    part_index = obc_blk_find_part_by_name(&pstAbi->stBlk, pName);
    if (part_index < 0)
    {
        printf("find part error %s\n", pName);
        return -1;
    }
    parts = pstAbi->stBlk.stParts_mmc;
    block_size = gstBlkDev->blksz;
    if (parts[part_index].addr % block_size ||
        parts[part_index].size % block_size)
    {
        printf("Partition %s is not aligned to device block size %u\n",
               pName, block_size);
        return -1;
    }
    if (obc_prepare_write_image(fileaddr, file_size, &image) != 0)
        return -1;
    if (image.has_header && image.head_write_flag != 0)
    {
        printf("Rootfs OBC package must use head_write_flag=0\n");
        return -1;
    }

    start = parts[part_index].addr / block_size;
    count = parts[part_index].size / block_size;
    if (image.payload_size >= sizeof(sparse_header_t) &&
        le32_to_cpu(((sparse_header_t *)image.payload)->magic) ==
        SPARSE_HEADER_MAGIC)
    {
        if (obc_validate_sparse(image.payload, image.payload_size, block_size,
                                parts[part_index].size) != 0)
            return -1;

        memset(&sparse, 0, sizeof(sparse));
        sparse.blksz = block_size;
        sparse.start = start;
        sparse.size = count;
        sparse.priv = gstBlkDev;
        sparse.write = obc_sparse_write;
        sparse.reserve = obc_sparse_reserve;
        if (write_sparse_image(&sparse, pName, image.payload, NULL) != 0)
        {
            printf("Sparse write to partition %s failed\n", pName);
            return -1;
        }
    }
    else
    {
        if (image.payload_size < 1082)
        {
            printf("Raw rootfs image is too small\n");
            return -1;
        }
        ext_magic = image.payload[1080] | ((u16)image.payload[1081] << 8);
        if (ext_magic != 0xef53)
        {
            printf("Raw rootfs image is not an ext filesystem\n");
            return -1;
        }
        if (image.payload_size > parts[part_index].size ||
            image.payload_size % block_size)
        {
            printf("Raw rootfs image size is invalid for partition %s\n", pName);
            return -1;
        }
        if (blk_dwrite(gstBlkDev, start, image.payload_size / block_size,
                       image.payload) != image.payload_size / block_size)
        {
            printf("Raw rootfs write to partition %s failed\n", pName);
            return -1;
        }
    }

    printf("Partition %s written successfully\n\n", pName);
    return 0;
}
#endif

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
