# Third-party tools build configuration
OBC_TOP_DIR ?= $(abspath $(CURDIR)/../..)
EXTERNEL_DIR := $(CURDIR)
sinclude $(OBC_TOP_DIR)/.config

TOOLCHAIN_PREFIX ?= $(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
CROSS_COMPILE ?= $(TOOLCHAIN_PREFIX)
HOST ?= $(patsubst %-,%,$(CROSS_COMPILE))
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar
RANLIB := $(CROSS_COMPILE)ranlib
TOOLCHAIN_SYSROOT ?= $(shell gcc=$(shell command -v $(CC) 2>/dev/null); \
	root=$$(cd "$$(dirname "$$gcc")/.." 2>/dev/null && pwd); \
	triple=$(patsubst %-,%,$(CROSS_COMPILE)); \
	for candidate in "$$root/$$triple/sysroot" "$$root"/*/sysroot; do \
		if [ -d "$$candidate" ]; then echo "$$candidate"; break; fi; \
	done)
TOOLCHAIN_CFLAGS := $(if $(TOOLCHAIN_SYSROOT),--sysroot=$(TOOLCHAIN_SYSROOT),)
export TOOLCHAIN_SYSROOT

# tools dir
TOOLS_IPERF_DIR     := $(EXTERNEL_DIR)/iperf
TOOLS_MBW_DIR   	:= $(EXTERNEL_DIR)/mbw
TOOLS_GDB_DIR   	:= $(EXTERNEL_DIR)/gdb
TOOLS_AUDIT_DIR   	:= $(EXTERNEL_DIR)/audit
TOOLS_BUSYBOX_DIR   := $(EXTERNEL_DIR)/busybox
TOOLS_V4L_UTILS_DIR   := $(EXTERNEL_DIR)/v4l-utils

# maek list
.PHONY: all clean
all : iperf mbw gdb audit busybox v4l_utils
clean: iperf_clean mbw_clean gdb_clean audit_clean busybox_clean v4l_utils_clean

# tools
.PHONY : iperf iperf_clean
ifeq ($(CONFIG_TOOLS_IPERF), y)
IPERF_BUILD_DIR = $(shell mkdir -p $(TOOLS_IPERF_DIR)/iperf-3.18/build  && cd $(TOOLS_IPERF_DIR)/iperf-3.18/build  && pwd)
iperf :
	if [ ! -e $(TOOLS_IPERF_DIR)/iperf-3.18/configure ] || [ ! -e $(TOOLS_IPERF_DIR)/iperf-3.18/Makefile ]; then \
		rm -rf $(TOOLS_IPERF_DIR)/iperf-3.18; \
		cd $(TOOLS_IPERF_DIR) && tar -xvf iperf-3.18.tar.gz; \
	fi
	cd $(TOOLS_IPERF_DIR)/iperf-3.18 && \
		CFLAGS="$(TOOLCHAIN_CFLAGS)" ./configure --prefix=$(IPERF_BUILD_DIR) \
		CC=$(CC) --host=$(HOST)
	$(MAKE) -C $(TOOLS_IPERF_DIR)/iperf-3.18 all install

iperf_clean :
	if [ -f $(TOOLS_IPERF_DIR)/iperf-3.18/Makefile ]; then \
		$(MAKE) -C $(TOOLS_IPERF_DIR)/iperf-3.18 clean distclean || true; \
	fi
	rm -rf $(TOOLS_IPERF_DIR)/iperf-3.18/build
endif


.PHONY : mbw mbw_clean
ifeq ($(CONFIG_TOOLS_MBW), y)
mbw :
	if [ ! -e $(TOOLS_MBW_DIR)/mbw-master/Makefile ]; then \
		cd $(TOOLS_MBW_DIR) && unzip mbw-master.zip; \
	fi
	cd $(TOOLS_MBW_DIR)/mbw-master && make

mbw_clean :
	make -C $(TOOLS_MBW_DIR)/mbw-master clean distclean
endif

.PHONY : gdb gdb_clean
ifeq ($(CONFIG_TOOLS_GDB), y)
TOOLS_GDB_VER=10.2
# MAKEINFO=/usr/bin/makeinfo
GDB_BUILD_DIR = $(shell mkdir -p $(TOOLS_GDB_DIR)/gdb-$(TOOLS_GDB_VER)/build  && cd $(TOOLS_GDB_DIR)/gdb-$(TOOLS_GDB_VER)/build  && pwd)
gdb :
	if [ ! -e $(TOOLS_GDB_DIR)/gdb-$(TOOLS_GDB_VER)/configure ]; then \
		cd $(TOOLS_GDB_DIR) && tar -xvf gdb-$(TOOLS_GDB_VER).tar.gz; \
	fi
	cd $(TOOLS_GDB_DIR)/gdb-$(TOOLS_GDB_VER) && ./configure --prefix=$(GDB_BUILD_DIR) CC=$(CC) --host=$(HOST)
	make -C $(TOOLS_GDB_DIR)/gdb-$(TOOLS_GDB_VER) all-gdb install-gdb

gdb_clean :
	make -C $(TOOLS_GDB_DIR) clean distclean
	rm $(GDB_BUILD_DIR) -rf
endif

.PHONY : audit audit_clean
ifeq ($(CONFIG_TOOLS_AUDIT), y)
TOOLS_AUDIT_VER=3.1.4
AUDIT_BUILD_DIR = $(shell mkdir -p $(TOOLS_AUDIT_DIR)/audit-$(TOOLS_AUDIT_VER)/build  && cd $(TOOLS_AUDIT_DIR)/audit-$(TOOLS_AUDIT_VER)/build  && pwd)
audit :
	if [ ! -e $(TOOLS_AUDIT_DIR)/audit-$(TOOLS_AUDIT_VER)/configure ]; then \
		cd $(TOOLS_AUDIT_DIR) && tar -xvf audit-$(TOOLS_AUDIT_VER).tar.gz; \
	fi
	cd $(TOOLS_AUDIT_DIR)/audit-$(TOOLS_AUDIT_VER) && ./configure --prefix=$(AUDIT_BUILD_DIR) CC=$(CC) --host=$(HOST) \
																						--without-python \
																						--without-python3 \
																						--disable-zos-remote \
																						--without-libcap-ng
	make -C $(TOOLS_AUDIT_DIR)/audit-$(TOOLS_AUDIT_VER) all install

audit_clean :
	make -C $(TOOLS_AUDIT_DIR) clean distclean
	rm $(AUDIT_BUILD_DIR) -rf
endif

.PHONY : busybox busybox_clean
ifeq ($(CONFIG_TOOLS_BUSYBOX), y)
TOOLS_BUSYBOX_VER=1.36.1
#Busybox fails to build with linux kernels >= 6.8,把tc.c从networking文件夹里移出去即可
BUSYBOX_BUILD_DIR = $(shell mkdir -p $(TOOLS_BUSYBOX_DIR)/busybox-$(TOOLS_BUSYBOX_VER)/build  && cd $(TOOLS_BUSYBOX_DIR)/busybox-$(TOOLS_BUSYBOX_VER)/build  && pwd)
busybox :
	if [ ! -e $(TOOLS_BUSYBOX_DIR)/busybox-$(TOOLS_BUSYBOX_VER) ]; then \
		cd $(TOOLS_BUSYBOX_DIR) && tar -xjf  busybox-$(TOOLS_BUSYBOX_VER).tar.bz2; \
	fi
	cd $(TOOLS_BUSYBOX_DIR)/busybox-$(TOOLS_BUSYBOX_VER)
	rm networking/tc.c
	make -C $(TOOLS_BUSYBOX_DIR)/busybox-$(TOOLS_BUSYBOX_VER) defconfig all install

gdb_clean :
	make -C $(TOOLS_BUSYBOX_DIR) clean distclean
	rm $(BUSYBOX_BUILD_DIR) -rf
endif

.PHONY : v4l_utils v4l_utils_clean
ifeq ($(CONFIG_TOOLS_V4L_UTILS), y)
TOOLS_V4L_UTILS_VER=1.22.1
#Busybox fails to build with linux kernels >= 6.8,把tc.c从networking文件夹里移出去即可
V4L_UTILS_BUILD_DIR = $(shell mkdir -p $(TOOLS_V4L_UTILS_DIR)/v4l-utils-$(TOOLS_V4L_UTILS_VER)/build  && cd $(TOOLS_V4L_UTILS_DIR)/v4l-utils-$(TOOLS_V4L_UTILS_VER)/build  && pwd)
v4l_utils :
	if [ ! -e $(TOOLS_V4L_UTILS_DIR)/v4l-utils-$(TOOLS_V4L_UTILS_VER)/configure ]; then \
		cd $(TOOLS_V4L_UTILS_DIR) && tar -xjf  v4l-utils-$(TOOLS_V4L_UTILS_VER).tar.bz2; \
	fi
	cd $(TOOLS_V4L_UTILS_DIR)/v4l-utils-$(TOOLS_V4L_UTILS_VER) && ./configure --prefix=$(V4L_UTILS_BUILD_DIR) CC=$(CC) --host=$(HOST) CFLAGS="-D_TIME_BITS=32" \
																										--without-libudev \
																										--disable-libdvbv5 \
																										--enable-v4l2-ctl-32 \
																										--with-udevdir=$(TOOLS_V4L_UTILS_DIR)/v4l-utils-$(TOOLS_V4L_UTILS_VER)/build/lib
	make -C $(TOOLS_V4L_UTILS_DIR)/v4l-utils-$(TOOLS_V4L_UTILS_VER) all install

v4l_utils_clean :
	make -C $(TOOLS_V4L_UTILS_DIR) clean distclean
	rm $(V4L_UTILS_BUILD_DIR) -rf
endif


# Kconfig file
KCONFIG_FILE := Kconfig

# menuconfig
.PHONY: menuconfig
menuconfig:
	kconfig-mconf $(KCONFIG_FILE)
