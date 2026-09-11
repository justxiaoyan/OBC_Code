#!/bin/sh

# ================= Configuration =================
# 从环境变量获取路径配置（由Makefile传入）
# PACK_TOP_DIR: SDK顶层目录
# PACK_RESOURCE_DIR: 资源目录（已拷贝好的源文件）
# PACK_RELEASE_DIR: 发布目录（打包后的输出文件）

# 用于记录成功和跳过的文件
PACKED_FILES=""
SKIPPED_FILES=""
PACK_FAILED_FILES=""

# ==========================================
# 文件名定义（在resource目录下的子目录中）
# ==========================================
FILE_LOADER="loader/tiboot3.bin"
FILE_FDT="fdt/k3-am625-sk.dtb"
FILE_TEEOS="uboot/tispl.bin"
FILE_UBOOT="uboot/u-boot.img"
FILE_KERNEL="linux/Image.gz"
FILE_ROOTFS="rootfs/rootfs.ext2"
FILE_APPFS="rootfs/appfs.ext2"

# ==========================================
# 输出文件名定义
# ==========================================
OUT_LOADER="100p-loader.bin"
OUT_FDT="100p-fdt.bin"
OUT_TEEOS="100p-teeos.bin"
OUT_UBOOT="100p-uboot.bin"
OUT_KERNEL="100p-kernel.bin"
OUT_ROOTFS="100p-rootfs.bin"
OUT_APPFS="100p-appfs.bin"

# ==========================================
# 打包配置表 (格式: "源文件变量名 输出文件变量名 head_write_flag")
# head_write_flag: yes=需要写入头部到分区, no=不需要写入头部
# ==========================================
PACK_TABLE="
FILE_LOADER    OUT_LOADER    no
FILE_FDT       OUT_FDT       yes
FILE_TEEOS     OUT_TEEOS     yes
FILE_UBOOT     OUT_UBOOT     yes
FILE_KERNEL    OUT_KERNEL    yes
FILE_ROOTFS    OUT_ROOTFS    no
FILE_APPFS     OUT_APPFS     no
"

# ================= Path Initialization =================
SCRIPT_DIR=$(cd "$(dirname "$0")" || exit; pwd)

# 使用Makefile传入的路径，如果没有传入则使用默认值
if [ -z "$PACK_RESOURCE_DIR" ]; then
    SOURCE_DIR="$SCRIPT_DIR/resource"
else
    SOURCE_DIR="$PACK_RESOURCE_DIR"
fi

if [ -z "$PACK_RELEASE_DIR" ]; then
    RELEASE_DIR="$SCRIPT_DIR/release"
else
    RELEASE_DIR="$PACK_RELEASE_DIR"
fi

if [ -z "$PACK_TOP_DIR" ]; then
    PACK_TOOL="$SCRIPT_DIR/../tools/sign_tools/output/ems_sign"
else
    PACK_TOOL="$PACK_TOP_DIR/tools/sign_tools/output/ems_sign"
fi

# ================= Functions =================

# Clean function: delete release directory
do_clean() {
    echo "[Clean] Starting cleanup..."
    if [ -d "$RELEASE_DIR" ]; then
        rm -rf "$RELEASE_DIR"
        echo "[Clean] Deleted directory: $RELEASE_DIR"
    else
        echo "[Clean] Directory not found, skipped: $RELEASE_DIR"
    fi
    echo "[Clean] Cleanup completed."
    exit 0
}

# Build function: execute pack logic
do_build() {
    # 1. Check if source directory exists (资源目录由Makefile已拷贝好)
    if [ ! -d "$SOURCE_DIR" ]; then
        echo "[Error] Source directory does not exist!"
        echo "Path: $SOURCE_DIR"
        exit 1
    fi

    # 2. Check if pack tool exists
    if [ ! -f "$PACK_TOOL" ]; then
        echo "[Error] Pack tool not found!"
        echo "Path: $PACK_TOOL"
        exit 1
    fi

    # 3. Create release output directory
    if [ ! -d "$RELEASE_DIR" ]; then
        echo "[Info] Creating release directory..."
        mkdir -p "$RELEASE_DIR"
    fi

    # 4. Execute pack commands
    echo "[Pack] Starting to pack files using pack tool..."
    echo "=========================================="

    # 使用临时文件记录结果（避免子shell变量丢失问题）
    PACKED_LIST="/tmp/packed_$$.txt"
    SKIPPED_LIST="/tmp/skipped_$$.txt"
    FAILED_LIST="/tmp/failed_$$.txt"
    > "$PACKED_LIST"
    > "$SKIPPED_LIST"
    > "$FAILED_LIST"

    echo "$PACK_TABLE" | while read -r src_var out_var head_flag; do
        # 跳过空行
        [ -z "$src_var" ] && continue

        # 通过变量名获取实际文件名
        eval src_file=\$$src_var
        eval out_file=\$$out_var

        src_path="$SOURCE_DIR/$src_file"
        dst_path="$RELEASE_DIR/$out_file"

        # 检查源文件是否存在
        if [ ! -f "$src_path" ]; then
            echo "⏭️  跳过: $src_file - 文件不存在"
            echo "$src_file" >> "$SKIPPED_LIST"
            continue
        fi

        # 根据head_flag决定是否使用-h参数
        if [ "$head_flag" = "yes" ]; then
            echo "📦 打包: $src_file -> $out_file (head_write_flag=1)"
            "$PACK_TOOL" -h "$src_path" "$dst_path"
        else
            echo "📦 打包: $src_file -> $out_file (head_write_flag=0)"
            "$PACK_TOOL" "$src_path" "$dst_path"
        fi

        if [ $? -eq 0 ]; then
            echo "✅ 打包成功: $out_file"
            echo "$out_file" >> "$PACKED_LIST"
        else
            echo "❌ 打包失败: $out_file"
            echo "$out_file" >> "$FAILED_LIST"
        fi
        echo ""
    done

    # 从临时文件读取结果
    PACKED_FILES=$(cat "$PACKED_LIST")
    SKIPPED_FILES=$(cat "$SKIPPED_LIST")
    PACK_FAILED_FILES=$(cat "$FAILED_LIST")

    # 清理临时文件
    rm -f "$PACKED_LIST" "$SKIPPED_LIST" "$FAILED_LIST"

    echo "=========================================="
    echo "✅ 打包任务完成！"
    echo "=========================================="

    # 显示打包汇总信息
    echo ""
    echo "📊 打包汇总:"
    echo "----------------------------------------"

    if [ -n "$PACKED_FILES" ]; then
        echo "✅ 成功打包的文件:"
        for file in $PACKED_FILES; do
            echo "   ✓ $file"
        done
    else
        echo "⚠️  没有成功打包任何文件"
    fi

    echo ""
    if [ -n "$SKIPPED_FILES" ] || [ -n "$PACK_FAILED_FILES" ]; then
        echo "⏭️  跳过/失败的文件:"
        for file in $SKIPPED_FILES; do
            echo "   - $file (文件不存在)"
        done
        for file in $PACK_FAILED_FILES; do
            echo "   - $file (打包失败)"
        done
    fi

    echo "----------------------------------------"
    echo "[Info] 输出目录: $RELEASE_DIR"
    if [ -n "$PACKED_FILES" ]; then
        echo "[Info] 生成的文件列表:"
        ls -lh "$RELEASE_DIR"
    fi
    echo "=========================================="
}

# ================= Main Entry =================
case "$1" in
    clean)
        do_clean
        ;;
    *)
        do_build
        ;;
esac
