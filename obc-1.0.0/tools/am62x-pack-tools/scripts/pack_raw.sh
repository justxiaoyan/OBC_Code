#!/bin/bash

# Define filenames to be copied
FILE_TIBOOT="tiboot3.bin"
FILE_DTB="k3-am625-sk.dtb"
FILE_TISPL="tispl.bin"
FILE_UBOOT="u-boot.img"
FILE_KERNEL="Image.gz"
FILE_ROOTFS="rootfs.ext2"
FILE_APPFS="appfs.ext2"

# Define output filenames after packing
OUT_TIBOOT="100p-loader.bin"
OUT_FDT="100p-fdt.bin"
OUT_TEE="100p-teeos.bin"
OUT_UBOOT="100p-uboot.bin"
OUT_KERNEL="100p-kernel.bin"
OUT_ROOTFS="100p-rootfs.bin"
OUT_APPFS="100p-appfs.bin"

# 用于记录成功和跳过的文件
COPIED_FILES=()
SKIPPED_FILES=()


# Clean function: delete resource and release directories
do_build()
{
    # Helper function to copy file if exists
    copy_if_exists() {
        local src=$1
        local dst=$2
        local name=$3

        if [ -f "$src" ]; then
            cp "$src" "$dst"
            if [ $? -eq 0 ]; then
                echo "✅ 已拷贝: $name"
                COPIED_FILES+=("$name")
            else
                echo "❌ 拷贝失败: $name"
                SKIPPED_FILES+=("$name (拷贝失败)")
            fi
        else
            echo "⏭️  跳过: $name - 文件不存在: $src"
            SKIPPED_FILES+=("$name (文件不存在)")
        fi
    }

    echo "=========================================="
    echo "开始打包文件..."
    echo "=========================================="

    # Pack tiboot3.bin
    copy_if_exists "$PACK_RESOURCE_DIR/loader/$FILE_TIBOOT" "$PACK_RELEASE_DIR/$OUT_TIBOOT" "$OUT_TIBOOT"

    # Pack k3-am625-sk.dtb
    copy_if_exists "$PACK_RESOURCE_DIR/fdt/$FILE_DTB" "$PACK_RELEASE_DIR/$OUT_FDT" "$OUT_FDT"

    # Pack u-boot.img
    copy_if_exists "$PACK_RESOURCE_DIR/uboot/$FILE_UBOOT" "$PACK_RELEASE_DIR/$OUT_UBOOT" "$OUT_UBOOT"

    # Pack tispl.bin
    copy_if_exists "$PACK_RESOURCE_DIR/uboot/$FILE_TISPL" "$PACK_RELEASE_DIR/$OUT_TEE" "$OUT_TEE"

    # Pack Image
    copy_if_exists "$PACK_RESOURCE_DIR/linux/$FILE_KERNEL" "$PACK_RELEASE_DIR/$OUT_KERNEL" "$OUT_KERNEL"

    # Copy rootfs.ext2 (no signing needed, just rename)
    copy_if_exists "$PACK_RESOURCE_DIR/rootfs/$FILE_ROOTFS" "$PACK_RELEASE_DIR/$OUT_ROOTFS" "$OUT_ROOTFS"

    # Copy appfs.ext2 (if exists)
    copy_if_exists "$PACK_RESOURCE_DIR/appfs/$FILE_APPFS" "$PACK_RELEASE_DIR/$OUT_APPFS" "$OUT_APPFS"

    echo "=========================================="
    echo "✅ 打包任务完成！"
    echo "=========================================="

    # 显示打包汇总信息
    echo ""
    echo "📊 打包汇总:"
    echo "----------------------------------------"
    if [ ${#COPIED_FILES[@]} -gt 0 ]; then
        echo "✅ 成功拷贝的文件 (${#COPIED_FILES[@]}个):"
        for file in "${COPIED_FILES[@]}"; do
            echo "   ✓ $file"
        done
    else
        echo "⚠️  没有成功拷贝任何文件"
    fi

    echo ""
    if [ ${#SKIPPED_FILES[@]} -gt 0 ]; then
        echo "⏭️  跳过的文件 (${#SKIPPED_FILES[@]}个):"
        for file in "${SKIPPED_FILES[@]}"; do
            echo "   - $file"
        done
    fi

    echo "----------------------------------------"
    echo "[Info] 输出目录: $PACK_RELEASE_DIR"
    if [ ${#COPIED_FILES[@]} -gt 0 ]; then
        echo "[Info] 生成的文件列表:"
        ls -lh "$PACK_RELEASE_DIR"
    fi
    echo "=========================================="
}


do_build;
