# OBC SDK top-level dispatcher
OBC_TOP_DIR := $(abspath $(CURDIR))
export OBC_TOP_DIR
sinclude $(OBC_TOP_DIR)/.config

CONFIG_PLATFORM_CONFIG := $(subst ",,$(CONFIG_PLATFORM_CONFIG))
PLATFORM_NAME ?= $(CONFIG_PLATFORM_CONFIG)

include scripts/common.mk
include platform_config/platform.mk
include scripts/system.mk
include scripts/module.mk
include scripts/tools.mk

ifeq ($(PLATFORM_NAME),am62x)
include scripts/am62x/pack-signed.mk
include scripts/am62x/loader.mk
include scripts/am62x/uboot.mk
include scripts/am62x/kernel.mk
include scripts/am62x/rootfs.mk
else
unsupported_platform:
	@echo "ERROR: Platform '$(PLATFORM_NAME)' is not implemented by the current build framework"; exit 1
loader loader_build loader_build_install uboot uboot_build uboot_build_install kernel kernel_build kernel_build_install rootfs rootfs_build rootfs_build_install system system_clean module module_clean tools tools_clean pack-signed: unsupported_platform
endif

.PHONY: all clean menuconfig help platform loader loader_clean uboot uboot_clean \
	kernel kernel_clean rootfs rootfs_clean system system_clean module module_clean \
	tools tools_clean sign_tools sign_tools_clean mkkimg mkkimg_clean

all: loader uboot kernel rootfs system module tools

clean: loader_clean uboot_clean kernel_clean rootfs_clean system_clean module_clean tools_clean

menuconfig:
	@if [ -z "$(KCONFIG_MCONF)" ]; then \
		echo "ERROR: kconfig-mconf not found in PATH."; \
		echo "Install it with: sudo apt install kconfig-frontends"; \
		echo "Or run: make KCONFIG_MCONF=/path/to/kconfig-mconf menuconfig"; \
		exit 127; \
	fi
	@$(KCONFIG_MCONF) Kconfig

help:
	@echo "Targets:"
	@echo "  all            Build loader, uboot, kernel, rootfs, system, module and tools"
	@echo "  loader         Build and sign AM62x loader"
	@echo "  uboot          Build and sign AM62x U-Boot and tispl"
	@echo "  kernel         Build and sign AM62x kernel"
	@echo "  rootfs         Build and sign rootfs"
	@echo "  system         Build appfs and third-party components"
	@echo "  module         Build kernel modules"
	@echo "  tools          Build sign_tools and mkkimg"
	@echo "  sign_tools     Build EMS signing tools"
	@echo "  mkkimg         Build factory image packer/unpacker"
	@echo "  menuconfig     Configure OBC"
	@echo "  platform       Select platform configuration"
	@echo "  clean          Clean all generated outputs"

.DEFAULT_GOAL := help
