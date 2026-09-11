#!/bin/bash

# 脚本名称: ems_image.sh
# 功能描述: 基于配置表将完整固件（含三备份）裸写入 SD 卡指定偏移量

SOURCE_DIR=.

# 用于记录烧录成功和跳过的分区
FLASHED_PARTITIONS=()
SKIPPED_PARTITIONS=()

# ==========================================
# 板卡名称配置
# ==========================================
EMS_BOARD_NAME="100p"

# ==========================================
# 分区配置表 (格式: "分区名 固件变量名 偏移地址(十六进制)")
# 注意: 地址已根据新的设备树分区布局重新计算
# ==========================================
PARTITION_TABLE=(
    "loader0    TIBOOT_BIN     0x00000000"
    "loader1    TIBOOT_BIN     0x00400000"
    "fdt0       FDT_BIN        0x00900000"
    "fdt1       FDT_BIN        0x00980000"
    "fdt2       FDT_BIN        0x00A00000"
    "teeos0     TISPL_BIN      0x00A80000"
    "teeos1     TISPL_BIN      0x00E80000"
    "teeos2     TISPL_BIN      0x01280000"
    "uboot0     UBOOT_IMG      0x01680000"
    "uboot1     UBOOT_IMG      0x01A80000"
    "uboot2     UBOOT_IMG      0x01E80000"
    "kernel0    KERNEL_BIN     0x02280000"
    "kernel1    KERNEL_BIN     0x04280000"
    "kernel2    KERNEL_BIN     0x06280000"
)
# "rootfs0    ROOTFS_BIN     0x08280000"
# "rootfs1    ROOTFS_BIN     0x0A280000"
# "rootfs2    ROOTFS_BIN     0x0C280000"
# "appfs0     APPFS_BIN      0x0E280000"
# "appfs1     APPFS_BIN      0x1E280000"
# "appfs2     APPFS_BIN      0x2E280000"


# 定义固件文件名 (变量名需与上方配置表中的第二列对应)
TIBOOT_BIN="$SOURCE_DIR/${EMS_BOARD_NAME}-loader.bin"
FDT_BIN="$SOURCE_DIR/${EMS_BOARD_NAME}-fdt.bin"
TISPL_BIN="$SOURCE_DIR/${EMS_BOARD_NAME}-teeos.bin"
UBOOT_IMG="$SOURCE_DIR/${EMS_BOARD_NAME}-uboot.bin"
KERNEL_BIN="$SOURCE_DIR/${EMS_BOARD_NAME}-kernel.bin"
ROOTFS_BIN="$SOURCE_DIR/${EMS_BOARD_NAME}-rootfs.bin"
APPFS_BIN="$SOURCE_DIR/${EMS_BOARD_NAME}-appfs.bin"

# 检查是否传入了 SD 卡设备参数
if [ -z "$1" ]; then
    echo "❌ 错误: 请指定目标 SD 卡设备！"
    echo "使用示例: sudo sh ${EMS_BOARD_NAME}_ems_image.sh /dev/sdd [partition_type]"
    echo ""
    echo "可选的分区类型:"
    echo "  loader  - 只烧录 loader0 和 loader1"
    echo "  fdt     - 只烧录 fdt0, fdt1 和 fdt2"
    echo "  teeos   - 只烧录 teeos0, teeos1 和 teeos2"
    echo "  uboot   - 只烧录 uboot0, uboot1 和 uboot2"
    echo "  kernel  - 只烧录 kernel0, kernel1 和 kernel2"
    echo "  rootfs  - 只烧录 rootfs0, rootfs1 和 rootfs2"
    echo "  appfs   - 只烧录 appfs0, appfs1 和 appfs2"
    echo "  all     - 烧录所有分区（默认）"
    exit 1
fi

TARGET_DEV=$1
PARTITION_TYPE=${2:-all}  # 默认为 all

# 验证分区类型参数
case "$PARTITION_TYPE" in
    loader|fdt|teeos|uboot|kernel|rootfs|appfs|all)
        ;;
    *)
        echo "❌ 错误: 无效的分区类型 '$PARTITION_TYPE'"
        echo "支持的类型: loader, fdt, teeos, uboot, kernel, rootfs, appfs, all"
        exit 1
        ;;
esac

# 安全检查：防止误写入本地硬盘
if [[ "$TARGET_DEV" == "/dev/sda" ]]; then
    echo "❌ 严重警告: 禁止将设备指定为 /dev/sda (通常是系统主硬盘)！操作已终止。"
    exit 1
fi

if [ ! -b "$TARGET_DEV" ]; then
    echo "❌ 错误: 设备 $TARGET_DEV 不存在或不是块设备，请检查 SD 卡是否插好。"
    exit 1
fi

echo "⚠️  即将开始烧录，目标设备: $TARGET_DEV"
echo "⚠️  烧录类型: $PARTITION_TYPE"
echo "⚠️  该操作将覆盖 $TARGET_DEV 的指定区域，请确保已卸载相关分区！"
read -p "确认继续吗? (y/n): " confirm
if [ "$confirm" != "y" ]; then
    echo "操作已取消。"
    exit 0
fi

# 判断分区是否需要烧录
should_flash_partition() {
    local part_name=$1

    # 如果是 all，烧录所有分区
    if [ "$PARTITION_TYPE" == "all" ]; then
        return 0
    fi

    # 根据分区类型判断
    case "$PARTITION_TYPE" in
        loader)
            [[ "$part_name" == loader* ]] && return 0
            ;;
        fdt)
            [[ "$part_name" == fdt* ]] && return 0
            ;;
        teeos)
            [[ "$part_name" == teeos* ]] && return 0
            ;;
        uboot)
            [[ "$part_name" == uboot* ]] && return 0
            ;;
        kernel)
            [[ "$part_name" == kernel* ]] && return 0
            ;;
        rootfs)
            [[ "$part_name" == rootfs* ]] && return 0
            ;;
        appfs)
            [[ "$part_name" == appfs* ]] && return 0
            ;;
    esac

    return 1
}

# 通用烧录函数
# 参数: $1=分区名, $2=固件文件路径, $3=偏移地址(十六进制)
flash_partition() {
    local part_name=$1
    local file_path=$2
    local offset_hex=$3

    # 检查固件文件是否存在
    if [ ! -f "$file_path" ]; then
        echo "⏭️  跳过 [$part_name] - 文件不存在: $file_path"
        SKIPPED_PARTITIONS+=("$part_name")
        return 0
    fi

    # 计算偏移量 (将十六进制转为十进制，再除以 1024 得到以 K 为单位的 seek 值)
    local offset_dec=$(($offset_hex))
    local seek_k=$((offset_dec / 1024))

    echo "🚀 正在烧录 [$part_name] -> $file_path (偏移: $offset_hex)"

    # 针对 rootfs 和 appfs 等大文件使用 1M 块大小加速，其他小文件使用 1K
    local block_size="1K"
    if [[ "$part_name" == rootfs* ]] || [[ "$part_name" == appfs* ]]; then
        block_size="1M"
        seek_k=$((offset_dec / 1024 / 1024))
        echo "⚠️  $part_name 文件较大，请耐心等待..."
    fi

    sudo dd if="$file_path" of="$TARGET_DEV" bs="$block_size" seek="$seek_k" conv=fsync status=progress

    if [ $? -eq 0 ]; then
        echo "✅ [$part_name] 烧录完成！\n"
        FLASHED_PARTITIONS+=("$part_name")
    else
        echo "❌ [$part_name] 烧录失败！\n"
        SKIPPED_PARTITIONS+=("$part_name (烧录失败)")
    fi
}

# 循环遍历分区表并执行烧录
echo "=========================================="
echo "开始根据分区表执行烧录任务..."
echo "=========================================="
for entry in "${PARTITION_TABLE[@]}"; do
    # 将配置行拆分为 分区名、变量名、偏移量
    read -r part_name var_name offset_hex <<< "$entry"

    # 检查是否需要烧录此分区
    if ! should_flash_partition "$part_name"; then
        echo "⏭️  跳过 [$part_name] (不在烧录范围内)"
        continue
    fi

    # 通过变量名获取实际的文件路径 (例如将 TIBOOT_BIN 转换为 $TIBOOT_BIN 的值)
    file_path="${!var_name}"

    # 调用烧录函数
    flash_partition "$part_name" "$file_path" "$offset_hex"
done

# 强制同步，确保所有数据从缓存刷入物理 SD 卡
sync

echo "=========================================="
echo "✅ 烧录任务完成！"
echo "=========================================="

# 显示烧录汇总信息
echo ""
echo "📊 烧录汇总:"
echo "----------------------------------------"
if [ ${#FLASHED_PARTITIONS[@]} -gt 0 ]; then
    echo "✅ 成功烧录的分区 (${#FLASHED_PARTITIONS[@]}个):"
    for part in "${FLASHED_PARTITIONS[@]}"; do
        echo "   ✓ $part"
    done
else
    echo "⚠️  没有成功烧录任何分区"
fi

echo ""
if [ ${#SKIPPED_PARTITIONS[@]} -gt 0 ]; then
    echo "⏭️  跳过的分区 (${#SKIPPED_PARTITIONS[@]}个):"
    for part in "${SKIPPED_PARTITIONS[@]}"; do
        echo "   - $part"
    done
fi
echo "=========================================="