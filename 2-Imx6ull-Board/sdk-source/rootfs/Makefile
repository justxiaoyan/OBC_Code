

### GLIBC Cross Compile AArch64
EXTERNEL_DIR 	:= $(shell pwd)
CROSS_COMPILE 	?= arm-buildroot-linux-gnueabihf-
CC           	:= $(CROSS_COMPILE)-gcc
CXX          	:= $(CROSS_COMPILE)-g++


# rootfs dir
ROOTFS_BUSYBOX_DIR   := $(EXTERNEL_DIR)/busybox

# make config
-include .config
ROOTFS_BUSYBOX_VER=1.36.1

# maek list
.PHONY: all clean
all : busybox rootfs_pack
clean: busybox_clean


.PHONY : busybox busybox_clean
ifeq ($(CONFIG_ROOTFS_BUSYBOX), y)
#Busybox fails to build with linux kernels >= 6.8,把tc.c从networking文件夹里移出去即可
BUSYBOX_BUILD_DIR = $(shell mkdir -p $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/  && cd $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/build  && pwd)
busybox :
	if [ ! -e $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER) ]; then \
		cd $(ROOTFS_BUSYBOX_DIR) && tar -xjf  busybox-$(ROOTFS_BUSYBOX_VER).tar.bz2; \
	fi

	cd $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)
	cp $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/../config/imx6ull_1.36.1_defconfig $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/.config

	if [ -e $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/networking/tc.c ]; then \
		rm $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/networking/tc.c; \
	fi
	make -C $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER) all install

gdb_clean :
	make -C $(ROOTFS_BUSYBOX_DIR) clean distclean
	rm $(BUSYBOX_BUILD_DIR) -rf
endif


# base_rootfs to rootfs
.PHONY : rootfs_pack
rootfs_pack:
	if [ -e $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/build/rootfs ]; then \
		cp -a $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)/build/rootfs/* $(ROOTFS_BUSYBOX_DIR)/../base_rootfs/; \
	fi
	sudo sh pack_rootfs.sh

# Kconfig file
KCONFIG_FILE := Kconfig

# menuconfig
.PHONY: menuconfig
menuconfig:
	kconfig-mconf $(KCONFIG_FILE)


