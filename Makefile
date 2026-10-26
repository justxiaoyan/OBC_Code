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
include scripts/factory.mk

ifeq ($(PLATFORM_NAME),am62x)
include scripts/pack-signed.mk
include scripts/am62x/loader.mk
include scripts/am62x/uboot.mk
include scripts/am62x/kernel.mk
include scripts/am62x/rootfs.mk
PLATFORM_ALL_TARGETS := $(if $(filter y,$(CONFIG_BUILD_LOADER)),loader) \
	$(if $(filter y,$(CONFIG_BUILD_UBOOT)),uboot) \
	$(if $(filter y,$(CONFIG_BUILD_KERNEL)),kernel) \
	$(if $(filter y,$(CONFIG_BUILD_ROOTFS)),rootfs) system module
ifeq ($(CONFIG_BUILD_LOADER)$(CONFIG_BUILD_UBOOT)$(CONFIG_BUILD_KERNEL)$(CONFIG_BUILD_ROOTFS),yyyy)
PLATFORM_ALL_TARGETS += factory
endif
else ifeq ($(PLATFORM_NAME),imx6ull)
include scripts/pack-signed.mk
include scripts/imx6ull/loader.mk
include scripts/imx6ull/uboot.mk
include scripts/imx6ull/kernel.mk
include scripts/imx6ull/rootfs.mk
PLATFORM_ALL_TARGETS := $(if $(filter y,$(CONFIG_BUILD_UBOOT)),uboot) \
	$(if $(filter y,$(CONFIG_BUILD_KERNEL)),kernel) \
	$(if $(filter y,$(CONFIG_BUILD_ROOTFS)),rootfs) system module
ifeq ($(CONFIG_BUILD_UBOOT)$(CONFIG_BUILD_KERNEL)$(CONFIG_BUILD_ROOTFS),yyy)
PLATFORM_ALL_TARGETS += factory
endif
else ifeq ($(PLATFORM_NAME),rk3562)
include scripts/pack-signed.mk
include scripts/rk3562/uboot.mk
include scripts/rk3562/kernel.mk
include scripts/rk3562/rootfs.mk
PLATFORM_ALL_TARGETS := loader uboot kernel rootfs system module factory
else ifeq ($(PLATFORM_NAME),rv1106)
PLATFORM_ALL_TOOLS :=
PLATFORM_ALL_TARGETS := $(if $(filter y,$(CONFIG_BUILD_LOADER)),loader) \
	$(if $(filter y,$(CONFIG_BUILD_UBOOT)),uboot) \
	$(if $(filter y,$(CONFIG_BUILD_KERNEL)),kernel) \
	$(if $(filter y,$(CONFIG_BUILD_ROOTFS)),rootfs) system
unsupported_platform:
	@echo "ERROR: SDK-backed image builds are not implemented for rv1106"; exit 1
loader loader_build loader_build_install: unsupported_platform
uboot uboot_build uboot_build_install: unsupported_platform
kernel kernel_build kernel_build_install: unsupported_platform
rootfs rootfs_build rootfs_build_install: unsupported_platform
fdt_build fdt_build_clean pack-signed factory: unsupported_platform
else
PLATFORM_ALL_TOOLS := tools
PLATFORM_ALL_TARGETS := loader uboot kernel rootfs system module factory
unsupported_platform:
	@echo "ERROR: Platform '$(PLATFORM_NAME)' is not implemented by the current build framework"; exit 1
loader loader_build loader_build_install uboot uboot_build uboot_build_install fdt_build fdt_build_clean kernel kernel_build kernel_build_install rootfs rootfs_build rootfs_build_install system system_clean module module_clean tools tools_clean pack-signed factory factory_clean: unsupported_platform
endif

.PHONY: all clean menuconfig help platform loader loader_clean uboot uboot_clean \
	kernel kernel_clean rootfs rootfs_clean system system_clean module module_clean \
	tools tools_clean sign_tools sign_tools_clean mkkimg mkkimg_clean fdt_build fdt_build_clean \
	factory factory_clean

PLATFORM_ALL_TOOLS ?= tools
all: $(PLATFORM_ALL_TOOLS) $(PLATFORM_ALL_TARGETS)

# loader and U-Boot temporarily patch the shared SDK tree, so the aggregate
# build must not run those two stages concurrently.
.NOTPARALLEL: all

ifeq ($(PLATFORM_NAME),rv1106)
clean: system_clean
else
clean: loader_clean uboot_clean kernel_clean rootfs_clean system_clean module_clean tools_clean factory_clean
endif

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
	@echo "  all            Build all supported components for the selected platform"
	@echo "  loader         Build and sign the platform loader"
	@echo "  uboot          Build and sign U-Boot (and platform boot resources)"
	@echo "  kernel         Build and sign kernel and device tree"
	@echo "  rootfs         Build and sign the 32 MiB sparse ext4 rootfs"
	@echo "  system         Build appfs and third-party components"
	@echo "  module         Build kernel modules"
	@echo "  tools          Build sign_tools and mkkimg"
	@echo "  sign_tools     Build OBC signing tools"
	@echo "  mkkimg         Build factory image packer/unpacker"
	@echo "  factory        Pack signed image resources into output/image/<platform>-factory.bin"
	@echo "  factory_clean  Remove the platform factory image"
	@echo "  menuconfig     Configure OBC"
	@echo "  platform       Select platform configuration"
	@echo "  clean          Clean all generated outputs"

.DEFAULT_GOAL := help
