# Shared build paths and toolchain discovery.
CONFIG_OBC_SDK_ARCH ?= arm
CONFIG_OBC_SDK_COMP ?= arm-buildroot-linux-gnueabihf-
KCONFIG_MCONF ?= $(shell command -v kconfig-mconf 2>/dev/null)
OBC_TOOLCHAIN_PREFIX := $(subst ",,$(CONFIG_OBC_SDK_COMP))
OBC_CROSS_GCC := $(shell command -v $(OBC_TOOLCHAIN_PREFIX)gcc 2>/dev/null)
OBC_TOOLCHAIN_ROOT := $(if $(OBC_CROSS_GCC),$(abspath $(dir $(OBC_CROSS_GCC))/..),)
OBC_TOOLCHAIN_TRIPLE := $(patsubst %-,%,$(OBC_TOOLCHAIN_PREFIX))
OBC_TOOLCHAIN_SYSROOT := $(firstword $(wildcard $(OBC_TOOLCHAIN_ROOT)/$(OBC_TOOLCHAIN_TRIPLE)/sysroot $(OBC_TOOLCHAIN_ROOT)/*/sysroot))
OBC_TOOLCHAIN_CFLAGS := $(if $(OBC_TOOLCHAIN_SYSROOT),--sysroot=$(OBC_TOOLCHAIN_SYSROOT),)
ifeq ($(PLATFORM_NAME),am62x)
UBOOT_TOOLCHAIN_VARS := \
	CC="env GCC_EXEC_PREFIX=$(OBC_TOOLCHAIN_ROOT)/lib/gcc/ $(OBC_TOOLCHAIN_PREFIX)gcc" \
	CXX="env GCC_EXEC_PREFIX=$(OBC_TOOLCHAIN_ROOT)/lib/gcc/ $(OBC_TOOLCHAIN_PREFIX)g++" \
	AR="$(OBC_TOOLCHAIN_ROOT)/bin/$(OBC_TOOLCHAIN_TRIPLE)-gcc-ar-reloc" \
	NM="$(OBC_TOOLCHAIN_ROOT)/bin/$(OBC_TOOLCHAIN_TRIPLE)-gcc-nm-reloc" \
	RANLIB="$(OBC_TOOLCHAIN_ROOT)/bin/$(OBC_TOOLCHAIN_TRIPLE)-gcc-ranlib-reloc"
BL31_FILE := $(firstword $(wildcard $(OBC_TOOLCHAIN_ROOT)/firmware/bl31.bin))
TEE_FILE := $(firstword $(wildcard $(OBC_TOOLCHAIN_ROOT)/firmware/tee-pager_v2.bin))
TI_DM_FILE := $(firstword $(wildcard $(OBC_TOOLCHAIN_ROOT)/firmware/ti-dm/*.xer5f))
ifneq ($(strip $(BL31_FILE)),)
export BL31 := $(BL31_FILE)
endif
ifneq ($(strip $(TEE_FILE)),)
export TEE := $(TEE_FILE)
endif
ifneq ($(strip $(TI_DM_FILE)),)
export TI_DM := $(TI_DM_FILE)
endif
ifneq ($(strip $(BINMAN_INDIRS)),)
UBOOT_TOOLCHAIN_VARS += BINMAN_INDIRS=$(BINMAN_INDIRS)
endif
endif
OBC_OUTPUT_DIR := $(OBC_TOP_DIR)/output
OBC_PACK_DIR := $(OBC_OUTPUT_DIR)/tmp
OBC_PACK_IMAGE_DIR := $(OBC_OUTPUT_DIR)/image
OBC_APPFS_DIR := $(OBC_OUTPUT_DIR)/appfs
OBC_MODULE_DIR := $(OBC_OUTPUT_DIR)/modules
OBC_SYSTEM_DIR := $(OBC_TOP_DIR)/system
OBC_APPFS_SOURCE_DIR := $(OBC_SYSTEM_DIR)/appfs
OBC_THIRD_PART_DIR := $(OBC_SYSTEM_DIR)/third-part
ROOTFS_SOURCE_DIR := $(OBC_TOP_DIR)/rootfs
SIGN_TOOLS_DIR := $(OBC_TOP_DIR)/tools/sign_tools
SIGN_TOOL := $(SIGN_TOOLS_DIR)/output/obc_sign
UNSIGN_TOOL := $(SIGN_TOOLS_DIR)/output/unsign_demo
MKKIMG_DIR := $(OBC_TOP_DIR)/tools/mkkimg
MKKIMG := $(MKKIMG_DIR)/output/mkkimg
UMKKIMG := $(MKKIMG_DIR)/output/umkkimg
PLATFORM_NAME := $(patsubst "%",%,$(CONFIG_PLATFORM_CONFIG))
SDK_NAME := $(if $(PLATFORM_NAME),$(PLATFORM_NAME)_sdk_source,)
OBC_SDK_BASE_DIR ?= $(OBC_TOP_DIR)/../obc_sdk
OBC_SDK_DIR := $(shell for p in "$(OBC_SDK_BASE_DIR)/$(SDK_NAME)" "$(OBC_TOP_DIR)/../$(SDK_NAME)" "$(OBC_TOP_DIR)/../../$(SDK_NAME)"; do if [ -d "$$p" ]; then realpath "$$p"; break; fi; done)
UBOOT_SDK_DIR := $(OBC_SDK_DIR)/uboot
KERNEL_SDK_DIR := $(OBC_SDK_DIR)/kernel
BINMAN_INDIRS := $(firstword $(wildcard $(OBC_SDK_DIR)/ti-k3-boot-firmware-* $(OBC_SDK_DIR)/uboot/binman-fake))
ifeq ($(PLATFORM_NAME),am62x)
ifneq ($(strip $(BINMAN_INDIRS)),)
UBOOT_TOOLCHAIN_VARS += BINMAN_INDIRS=$(BINMAN_INDIRS)
endif
endif
CONFIG_OBC_SDK_ARCH := $(subst ",,$(CONFIG_OBC_SDK_ARCH))
CONFIG_OBC_SDK_COMP := $(subst ",,$(CONFIG_OBC_SDK_COMP))
CONFIG_KERNEL_ARCH := $(subst ",,$(CONFIG_KERNEL_ARCH))
CONFIG_UBOOT_BIN_NAME := $(subst ",,$(CONFIG_UBOOT_BIN_NAME))
CONFIG_LOADER_BIN_NAME := $(subst ",,$(CONFIG_LOADER_BIN_NAME))
CONFIG_TISPL_BIN_NAME := $(subst ",,$(CONFIG_TISPL_BIN_NAME))
CONFIG_FDT_BIN_NAME := $(subst ",,$(CONFIG_FDT_BIN_NAME))
CONFIG_KERNEL_BIN_NAME := $(subst ",,$(CONFIG_KERNEL_BIN_NAME))
CONFIG_ROOTFS_BIN_NAME := $(subst ",,$(CONFIG_ROOTFS_BIN_NAME))
CONFIG_R5_LOADER_DEFCONFIG := $(subst ",,$(CONFIG_R5_LOADER_DEFCONFIG))
CONFIG_R5_LOADER_BIN_NAME := $(subst ",,$(CONFIG_R5_LOADER_BIN_NAME))
export OBC_TOP_DIR OBC_OUTPUT_DIR OBC_PACK_DIR OBC_PACK_IMAGE_DIR OBC_APPFS_DIR OBC_MODULE_DIR
export OBC_TOOLCHAIN_PREFIX OBC_TOOLCHAIN_ROOT OBC_TOOLCHAIN_SYSROOT OBC_TOOLCHAIN_CFLAGS
export OBC_SDK_DIR UBOOT_SDK_DIR KERNEL_SDK_DIR BINMAN_INDIRS
.PHONY: check_config check_sdk output
check_config:
	@test -f "$(OBC_TOP_DIR)/.config" || { echo "ERROR: .config not found; run make platform"; exit 1; }
check_sdk: check_config
	@test -n "$(OBC_SDK_DIR)" && test -d "$(OBC_SDK_DIR)" || { echo "ERROR: SDK not found for platform $(PLATFORM_NAME)"; exit 1; }
	@test -d "$(UBOOT_SDK_DIR)" || { echo "ERROR: missing $(UBOOT_SDK_DIR)"; exit 1; }
	@test -d "$(KERNEL_SDK_DIR)" || { echo "ERROR: missing $(KERNEL_SDK_DIR)"; exit 1; }
output:
	@mkdir -p "$(OBC_PACK_DIR)" "$(OBC_PACK_IMAGE_DIR)" "$(OBC_APPFS_DIR)" "$(OBC_MODULE_DIR)"
