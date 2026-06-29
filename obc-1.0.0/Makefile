# OBC top dir
TFTP_DEBUG_DIR := "/home/ayan-server/tftp"

OBC_TOP_DIR := $(shell pwd)
export OBC_TOP_DIR
OBC_SDK_BASE_DIR := $(shell pwd)/../obc_sdk

# 默认值
CONFIG_OBC_SDK_ARCH ?= "arm"
CONFIG_OBC_SDK_COMP ?= "arm-buildroot-linux-gnueabihf-"

sinclude $(OBC_TOP_DIR)/.config

# 平台化配置路径
include platform_config/platform.mk
# module配置
include module/module.mk

# SDK目录配置（从git仓库获取）
SDK_NAME := $(patsubst "%",%,$(CONFIG_SDK_NAME))
OBC_SDK_DIR := $(OBC_SDK_BASE_DIR)/$(SDK_NAME)
UBOOT_SDK_DIR := $(OBC_SDK_DIR)/uboot
KERNEL_SDK_DIR := $(OBC_SDK_DIR)/kernel
ROOTFS_SDK_DIR := $(OBC_TOP_DIR)/rootfs

# 输出目录配置
OBC_OUTPUT_DIR 		:= $(OBC_TOP_DIR)/output
# 中间产物路径
OBC_PACK_DIR 		:= $(OBC_OUTPUT_DIR)/tmp
# 输出可以用于升级的路径
OBC_PACK_IMAGE_DIR 	:= $(OBC_OUTPUT_DIR)/image
# pack工具包路径
OBC_PACK_TOOLS_DIR 	:= $(OBC_OUTPUT_DIR)/tools
# appfs安装路径
OBC_APPFS_DIR 		:= $(OBC_OUTPUT_DIR)/appfs


.PHONY: all clean appfs appfs_clean menuconfig defconfig savedefconfig tools tools_clean check_sdk
all : output check_sdk uboot kernel rootfs
install: check_sdk uboot_install kernel_install rootfs_install appfs_install
clean: uboot_clean kernel_clean rootfs_clean pack_clean appfs_clean tools_clean


check_config:
	@if [ ! -f .config ]; then \
		echo "\r\nERROR: .config is not exit !!!\r\n"; \
		echo "please use'make platform' chioce platform\r\n"; \
		exit 1; \
	fi

check_sdk: check_config
	@echo "Checking SDK directory..."
	@SDK_NAME=$(patsubst "%",%,$(CONFIG_SDK_NAME)); \
	SDK_DIR=$(OBC_SDK_BASE_DIR)/$$SDK_NAME; \
	GIT_SOURCE=$(patsubst "%",%,$(CONFIG_GIT_SOURCE)); \
	if [ ! -d "$(OBC_SDK_BASE_DIR)" ]; then \
		echo "Creating SDK base directory: $(OBC_SDK_BASE_DIR)"; \
		mkdir -p $(OBC_SDK_BASE_DIR); \
	fi; \
	if [ ! -d "$$SDK_DIR" ]; then \
		echo "SDK directory not found, cloning from $$GIT_SOURCE..."; \
		cd $(OBC_SDK_BASE_DIR) && git clone $$GIT_SOURCE $$SDK_NAME; \
		if [ $$? -ne 0 ]; then \
			echo "ERROR: Failed to clone SDK repository"; \
			exit 1; \
		fi; \
		echo "SDK cloned successfully to $$SDK_DIR"; \
	else \
		echo "SDK directory exists: $$SDK_DIR"; \
	fi; \
	if [ ! -d "$$SDK_DIR/uboot" ]; then \
		echo "ERROR: uboot directory not found in SDK"; \
		exit 1; \
	fi; \
	if [ ! -d "$$SDK_DIR/kernel" ]; then \
		echo "ERROR: kernel directory not found in SDK"; \
		exit 1; \
	fi

output:
	$(shell mkdir -p $(OBC_PACK_DIR))
	$(shell mkdir -p $(OBC_PACK_IMAGE_DIR))
	$(shell mkdir -p $(OBC_PACK_TOOLS_DIR))
	$(shell mkdir -p $(OBC_APPFS_DIR))
	@if [ -f $(OBC_TOP_DIR)/tools/pack_tools/output/pack ]; then \
		cp $(OBC_TOP_DIR)/tools/pack_tools/output/pack $(OBC_PACK_TOOLS_DIR); \
		cp $(OBC_TOP_DIR)/tools/pack_tools/output/unpack_demo $(OBC_PACK_TOOLS_DIR); \
	else \
		echo "Error: no pack tools\r\n"; \
		exit 1; \
	fi

# 各个成果物编译
uboot:
	$(MAKE) -j $(shell expr $(shell nproc) - 1) -C $(UBOOT_SDK_DIR) ARCH=$(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH)) CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
	$(MAKE) -C $(OBC_TOP_DIR) uboot_install

kernel:
	$(MAKE) -j $(shell expr $(shell nproc) - 1) -C $(KERNEL_SDK_DIR) ARCH=$(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH)) CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
	$(MAKE) -C $(OBC_TOP_DIR) kernel_install

rootfs:
	$(MAKE) -C $(ROOTFS_SDK_DIR) rootfs_pack  ARCH=$(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH)) CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
	$(MAKE) -C $(OBC_TOP_DIR) rootfs_install

# 成果物清除路径
uboot_clean:
	$(MAKE) -C $(UBOOT_SDK_DIR) clean

kernel_clean:
	$(MAKE) -C $(KERNEL_SDK_DIR) clean

rootfs_clean:
	$(MAKE) -C $(ROOTFS_SDK_DIR) clean

# 成果物移动到output目标路径
uboot_install: check_sdk output
	@echo "Installing uboot to $(OBC_PACK_DIR)..."
	@if [ -f $(UBOOT_SDK_DIR)/$(patsubst "%",%,$(CONFIG_UBOOT_BIN_NAME)) ]; then \
		cp $(UBOOT_SDK_DIR)/$(patsubst "%",%,$(CONFIG_UBOOT_BIN_NAME)) $(OBC_PACK_DIR)/; \
		echo "  Installed: $(patsubst "%",%,$(CONFIG_UBOOT_BIN_NAME))"; \
	else \
		echo "  Warning: $(patsubst "%",%,$(CONFIG_UBOOT_BIN_NAME)) not found"; \
	fi
	@if [ -f $(UBOOT_SDK_DIR)/arch/arm/dts/$(patsubst "%",%,$(CONFIG_FDT_BIN_NAME)) ]; then \
		cp $(UBOOT_SDK_DIR)/arch/arm/dts/$(patsubst "%",%,$(CONFIG_FDT_BIN_NAME)) $(OBC_PACK_DIR)/; \
		echo "  Installed: $(patsubst "%",%,$(CONFIG_FDT_BIN_NAME))"; \
	else \
		echo "  Warning: $(patsubst "%",%,$(CONFIG_FDT_BIN_NAME)) not found"; \
	fi

kernel_install: check_sdk output
	@echo "Installing kernel to $(OBC_PACK_DIR)..."
	@if [ -f $(KERNEL_SDK_DIR)/arch/arm/boot/$(patsubst "%",%,$(CONFIG_KERNEL_BIN_NAME)) ]; then \
		cp $(KERNEL_SDK_DIR)/arch/arm/boot/$(patsubst "%",%,$(CONFIG_KERNEL_BIN_NAME)) $(OBC_PACK_DIR)/; \
		echo "  Installed: $(patsubst "%",%,$(CONFIG_KERNEL_BIN_NAME))"; \
	else \
		echo "  Warning: $(patsubst "%",%,$(CONFIG_KERNEL_BIN_NAME)) not found"; \
	fi

rootfs_install: check_sdk output
	@echo "Installing rootfs to $(OBC_PACK_DIR)..."
	@if [ -f $(ROOTFS_SDK_DIR)/$(patsubst "%",%,$(CONFIG_ROOTFS_BIN_NAME)) ]; then \
		cp $(ROOTFS_SDK_DIR)/$(patsubst "%",%,$(CONFIG_ROOTFS_BIN_NAME)) $(OBC_PACK_DIR)/; \
		echo "  Installed: $(patsubst "%",%,$(CONFIG_ROOTFS_BIN_NAME))"; \
	else \
		echo "  Warning: $(patsubst "%",%,$(CONFIG_ROOTFS_BIN_NAME)) not found"; \
	fi

# 将需要pack的成果物进行重新打包，目前就是fdt和kernel需要pack
pack: check_config output uboot_install kernel_install rootfs_install
	@echo "Packing images to $(OBC_PACK_IMAGE_DIR)..."
	@if [ -f $(OBC_PACK_DIR)/$(patsubst "%",%,$(CONFIG_UBOOT_BIN_NAME)) ]; then \
		cp $(OBC_PACK_DIR)/$(patsubst "%",%,$(CONFIG_UBOOT_BIN_NAME)) $(OBC_PACK_IMAGE_DIR)/uboot.bin; \
		echo "  Copied: uboot.bin"; \
		$(OBC_PACK_TOOLS_DIR)/pack $(OBC_PACK_DIR)/$(patsubst "%",%,$(CONFIG_FDT_BIN_NAME)) $(OBC_PACK_IMAGE_DIR)/fdt.dtb; \
	fi
	@if [ -f $(OBC_PACK_DIR)/$(patsubst "%",%,$(CONFIG_KERNEL_BIN_NAME)) ]; then \
		$(OBC_PACK_TOOLS_DIR)/pack $(OBC_PACK_DIR)/$(patsubst "%",%,$(CONFIG_KERNEL_BIN_NAME)) $(OBC_PACK_IMAGE_DIR)/zImage; \
	fi
	@if [ -f $(OBC_PACK_DIR)/$(patsubst "%",%,$(CONFIG_ROOTFS_BIN_NAME)) ]; then \
		cp $(OBC_PACK_DIR)/$(patsubst "%",%,$(CONFIG_ROOTFS_BIN_NAME)) $(OBC_PACK_IMAGE_DIR)/rootfs.bin; \
		echo "  Copied: rootfs.bin"; \
	fi
	@echo "Pack complete!"


pack_install: pack
	@cp $(OBC_PACK_IMAGE_DIR)/* $(TFTP_DEBUG_DIR)

pack_clean:
	rm $(OBC_PACK_DIR)/*;
	rm $(OBC_PACK_IMAGE_DIR)/*;

uboot_menuconfig:
	@make -C $(UBOOT_SDK_DIR) menuconfig

kernel_menuconfig:
	@make -C $(KERNEL_SDK_DIR) menuconfig ARCH=$(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH)) CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))

# appfs 应用程序编译
appfs: check_config
	@echo "Building appfs applications..."
	$(MAKE) -j $(shell expr $(shell nproc) - 1) -C $(OBC_TOP_DIR)/appfs all

appfs_install: appfs output
	@echo "Installing appfs applications to $(OBC_APPFS_DIR)..."
	$(MAKE) -C $(OBC_TOP_DIR)/appfs install

appfs_clean:
	@echo "Cleaning appfs..."
	$(MAKE) -C $(OBC_TOP_DIR)/appfs clean

# tools 工具编译
tools:
	@echo "Building tools..."
	$(MAKE) -C $(OBC_TOP_DIR)/tools/pack_tools all

tools_clean:
	@echo "Cleaning tools..."
	$(MAKE) -C $(OBC_TOP_DIR)/tools/pack_tools clean

# menuconfig 配置系统
menuconfig:
	@kconfig-mconf Kconfig

defconfig:
	@if [ -z "$(CONFIG_PLATFORM_CONFIG)" ]; then \
		echo "Error: CONFIG_PLATFORM_CONFIG not set. Please run 'make platform' first."; \
		exit 1; \
	fi
	@PLATFORM=$$(echo $(CONFIG_PLATFORM_CONFIG) | sed 's/"//g'); \
	if [ -f platform_config/$$PLATFORM/$${PLATFORM}_defconfig ]; then \
		cp platform_config/$$PLATFORM/$${PLATFORM}_defconfig .config; \
		echo "Loaded defconfig for $$PLATFORM"; \
	else \
		echo "Error: defconfig not found for $$PLATFORM"; \
		exit 1; \
	fi

savedefconfig:
	@if [ ! -f .config ]; then \
		echo "Error: .config not found. Run 'make menuconfig' first."; \
		exit 1; \
	fi
	@if [ -z "$(CONFIG_PLATFORM_CONFIG)" ]; then \
		echo "Error: CONFIG_PLATFORM_CONFIG not set in .config"; \
		exit 1; \
	fi
	@PLATFORM=$$(echo $(CONFIG_PLATFORM_CONFIG) | sed 's/"//g'); \
	cp .config platform_config/$$PLATFORM/$${PLATFORM}_defconfig; \
	echo "Saved .config to platform_config/$$PLATFORM/$${PLATFORM}_defconfig"

# 显示帮助信息
help:
	@echo "********************Command list:"
	@echo "  make all         			- 编译所有资源"
	@echo "  make install     			- 安装所有成果物到output目录"
	@echo "  make pack        			- 对需要添加头信息的资源进行打包"
	@echo "  make pack_install        	- 对需要添加头信息的资源进行打包 & 并填充到tftp目录"
	@echo "  make platform    			- 选择平台并拷贝对应的defconfig文件"
	@echo "  make menuconfig  			- 配置OBC SDK (包括appfs应用)"
	@echo "  make defconfig   			- 加载当前平台的默认配置"
	@echo "  make savedefconfig  		- 保存当前配置到平台defconfig文件"
	@echo "  make appfs       			- 编译appfs应用程序"
	@echo "  make appfs_install       	- 安装appfs应用程序到output/appfs"
	@echo "  make tools       			- 编译tools工具 (pack/unpack)"
	@echo "  make help        			- 显示此帮助信息"
	@echo ""
	@echo "Output directory: $(OBC_OUTPUT_DIR)"
	@echo "  - tmp/         : 中间成果物 (uboot, kernel, rootfs)"
	@echo "  - image/       : 打包后的镜像文件"
	@echo "  - tools/       : 打包工具"
	@echo "  - appfs/       : 应用程序"

	@echo "  make uboot/kernel/rootfs   - 编译指定资源"
	@echo "  make clean    				- 清除"


.PHONY: platform help
