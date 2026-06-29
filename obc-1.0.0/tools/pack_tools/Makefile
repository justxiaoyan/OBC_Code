


### GLIBC Cross Compile AArch64
EXTERNEL_DIR := $(shell pwd)
HOST         :=
CC           := $(HOST)-gcc
CXX          := $(HOST)-g++
LD           := $(HOST)-ld

# tools dir
TOOLS_PACK_DIR     := $(EXTERNEL_DIR)/pack

OBC_OUTPUT_DIR		   :=  $(shell mkdir -p $(EXTERNEL_DIR)/output  && cd $(EXTERNEL_DIR)/output && pwd)
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
pack :
	@echo "Building pack tools..."
	cd $(TOOLS_PACK_DIR)
	make -C $(TOOLS_PACK_DIR) all install
	@echo "Pack tools built successfully."

pack_clean :
	@echo "Cleaning pack tools..."
	make -C $(TOOLS_PACK_DIR) clean


# Kconfig file
KCONFIG_FILE := Kconfig

# menuconfig
.PHONY: menuconfig
menuconfig:
	kconfig-mconf $(KCONFIG_FILE)















