### GLIBC Cross Compile AArch64
EXTERNEL_DIR := $(shell pwd)
HOST         := arm-linux-gnueabihf
CC           := $(HOST)-gcc
CXX          := $(HOST)-g++


# tools dir
TOOLS_IPERF_DIR   := $(EXTERNEL_DIR)/iperf
TOOLS_MBW_DIR   	:= $(EXTERNEL_DIR)/mbw
TOOLS_GDB_DIR   	:= $(EXTERNEL_DIR)/gdb
TOOLS_AUDIT_DIR   	:= $(EXTERNEL_DIR)/audit

# make config
-include .config

# maek list
.PHONY: all clean
all : iperf mbw gdb audit
clean: iperf_clean mbw_clean gdb_clean audit_clean

# tools
.PHONY : iperf iperf_clean
ifeq ($(CONFIG_TOOLS_IPERF), y)
IPERF_BUILD_DIR = $(shell mkdir -p $(TOOLS_IPERF_DIR)/iperf-3.18/build  && cd $(TOOLS_IPERF_DIR)/iperf-3.18/build  && pwd)
iperf :
	if [ ! -e $(TOOLS_IPERF_DIR)/iperf-3.18/configure ]; then \
		cd $(TOOLS_IPERF_DIR) && tar -xvf iperf-3.18.tar.gz; \
	fi
	cd $(TOOLS_IPERF_DIR)/iperf-3.18 && ./configure --prefix=$(IPERF_BUILD_DIR) CC=$(CC) --host=$(HOST)
	make -C $(TOOLS_IPERF_DIR)/iperf-3.18 all install

iperf_clean :
	make -C $(TOOLS_IPERF_DIR) clean distclean
	rm $(IPERF_BUILD_DIR) -rf
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

# Kconfig file
KCONFIG_FILE := Kconfig

# menuconfig
.PHONY: menuconfig
menuconfig:
	kconfig-mconf $(KCONFIG_FILE)