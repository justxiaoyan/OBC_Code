


### GLIBC Cross Compile AArch64
EXTERNEL_DIR := $(shell pwd)
HOST         :=
CC           := $(HOST)-gcc
CXX          := $(HOST)-g++
LD           := $(HOST)-ld

# tools dir
TOOLS_EMS_SIGN_DIR := $(EXTERNEL_DIR)/ems_sign

OBC_OUTPUT_DIR := $(shell mkdir -p $(EXTERNEL_DIR)/output && cd $(EXTERNEL_DIR)/output && pwd)
export OBC_OUTPUT_DIR

# make config
-include .config

# make list
.PHONY: all clean
all : ems_sign
clean: ems_sign_clean
	rm -rf output

# tools
.PHONY : ems_sign ems_sign_clean

ems_sign :
	cd $(TOOLS_EMS_SIGN_DIR)
	make -C $(TOOLS_EMS_SIGN_DIR) all install

ems_sign_clean :
	make -C $(TOOLS_EMS_SIGN_DIR) clean



# Kconfig file
KCONFIG_FILE := Kconfig

# menuconfig
.PHONY: menuconfig
menuconfig:
	kconfig-mconf $(KCONFIG_FILE)















