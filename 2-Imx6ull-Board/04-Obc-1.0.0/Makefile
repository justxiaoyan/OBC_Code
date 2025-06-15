


### GLIBC Cross Compile AArch64
EXTERNEL_DIR := $(shell pwd)
HOST         :=
CC           := $(HOST)-gcc
CXX          := $(HOST)-g++
LD           := $(HOST)-ld

# tools dir
TOOLS_PACK_DIR     := $(EXTERNEL_DIR)/tools/pack

OBC_OUTPUT_DIR		   :=  $(shell mkdir -p $(EXTERNEL_DIR)/output/tools  && cd $(EXTERNEL_DIR)/output/tools && pwd)
export OBC_OUTPUT_DIR

# make config
-include .config

# maek list
.PHONY: all clean
all : pack
clean: pack_clean
	rm -rf output

# tools
.PHONY : pack pack_clean
ifeq ($(CONFIG_TOOLS_PACK), y)
pack :
	cd $(TOOLS_PACK_DIR)
	make -C $(TOOLS_PACK_DIR) all install

pack_clean :
	make -C $(TOOLS_PACK_DIR) clean
endif


# Kconfig file
KCONFIG_FILE := Kconfig

# menuconfig
.PHONY: menuconfig
menuconfig:
	kconfig-mconf $(KCONFIG_FILE)















