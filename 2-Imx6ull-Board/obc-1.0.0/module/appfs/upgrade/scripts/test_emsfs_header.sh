#!/bin/bash
# 测试 EMSFS 签名包解析

echo "=========================================="
echo " 测试 EMSFS 签名包解析"
echo "=========================================="
echo ""

RELEASE_DIR="/home/hushanyan/CSPD/buildroot/output/images/release"

if [ ! -d "$RELEASE_DIR" ]; then
    echo "错误: release 目录不存在"
    exit 1
fi

echo "[1] 查看 release 目录文件:"
ls -lh $RELEASE_DIR
echo ""

echo "[2] 查看 fdt-sign.bin 包头（前 512 字节）:"
hexdump -C $RELEASE_DIR/fdt-sign.bin | head -20
echo ""

echo "[3] 提取包头信息:"
echo "魔数: $(hexdump -C $RELEASE_DIR/fdt-sign.bin | head -1 | awk '{print $2$3$4$5$6$7}')"
echo ""

echo "[4] 文件映射关系:"
echo "  loader-sign.bin  → loader  升级"
echo "  uboot-sign.bin   → uboot   升级"
echo "  kernel-sign.bin  → kernel  升级"
echo "  fdt-sign.bin     → fdt     升级"
echo "  teeos-sign.bin   → teeos   升级"
echo ""

echo "[5] 使用示例:"
echo "  cd /srv/nfs/hushanyan/"
echo "  ./upgrade -fdt $RELEASE_DIR/fdt-sign.bin"
echo "  ./upgrade -kernel $RELEASE_DIR/kernel-sign.bin"
echo ""

echo "=========================================="
