#!/bin/bash

# 定义U-Boot和Kernel的路径
UBOOTA_BIN_DIR=/home/wanguo/52-Github-Code/OBC_Code/2-Imx6ull-Board/01-Uboot
IMX6ULL_KERNEL_DIR=/home/wanguo/52-Github-Code/OBC_Code/2-Imx6ull-Board/02-Kernel
TFTP_DIR="/home/wanguo/99-tftp"

# 定义文件名
IMX6ULL_FDT_NAME=imx6ull-14x14-evk.dtb
IMX6ULL_KERNEL_NAME=zImage

# 检查文件是否存在并拷贝
check_and_copy() {
    local src_file=$1
    local dest_dir="."

    if [ -f "$src_file" ]; then
        echo "File $src_file exists. Copying..."
        cp "$src_file" "$dest_dir"
        if [ $? -eq 0 ]; then
            echo "File copied successfully."
        else
            echo "Failed to copy file $src_file."
            exit 1
        fi
    else
        echo "Error: File $src_file does not exist."
        exit 1
    fi
}

# 打包函数
pack_resource() {
    local resource_name=$1
    local resource_file=$2
    local pack_dir="pack_file"

    # 检查打包目录是否存在，不存在则创建
    if [ ! -d "$pack_dir" ]; then
        mkdir -p "$pack_dir"
        if [ $? -ne 0 ]; then
            echo "Failed to create directory $pack_dir."
            exit 1
        fi
    fi

    # 调用check_and_copy函数拷贝文件
    check_and_copy "$resource_file"

    # 执行打包命令
    ./pack "$resource_name" "$pack_dir/$resource_name"
    if [ $? -eq 0 ]; then
        echo "Packed $resource_name successfully."
    else
        echo "Failed to pack $resource_name."
        exit 1
    fi

    cp $pack_dir/$resource_name $TFTP_DIR
    sync
}

# 根据传入的参数决定打包哪个资源
if [ "$1" = "fdt" ]; then
    # 定义FDT文件路径
    IMX6ULL_FDT_FILE=$UBOOTA_BIN_DIR/arch/arm/dts/$IMX6ULL_FDT_NAME
    pack_resource "$IMX6ULL_FDT_NAME" "$IMX6ULL_FDT_FILE"
elif [ "$1" = "kernel" ]; then
    # 定义Kernel文件路径
    IMX6ULL_KERNEL_FILE_DIR=$IMX6ULL_KERNEL_DIR/arch/arm/boot/$IMX6ULL_KERNEL_NAME
    pack_resource "$IMX6ULL_KERNEL_NAME" "$IMX6ULL_KERNEL_FILE_DIR"
else
    echo "Usage: sh pack.sh <fdt|kernel>"
    echo "Example: sh pack.sh fdt or sh pack.sh kernel"
fi

