#!/bin/sh

rm -rf rootfs
mkdir rootfs

touch rootfs.ext2
dd if=/dev/zero of=rootfs.ext2 bs=1M count=32

mkfs.ext2 rootfs.ext2

mount -o loop rootfs.ext2 rootfs

cp -a base_rootfs/* rootfs

umount rootfs.ext2
