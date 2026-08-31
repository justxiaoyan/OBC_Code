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

# SDK目录配置（根据平台配置动态生成）
PLATFORM_NAME := $(patsubst "%",%,$(CONFIG_PLATFORM_CONFIG))
SDK_NAME := $(PLATFORM_NAME)_sdk_source
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

# module配置（必须在 OBC_OUTPUT_DIR 定义之后）
include module/module.mk


.PHONY: all clean appfs appfs_clean module module_clean menuconfig defconfig savedefconfig tools tools_clean check_sdk
all : output check_sdk uboot kernel rootfs
install: check_sdk uboot_install kernel_install rootfs_install appfs_install module_install
clean: uboot_clean kernel_clean rootfs_clean pack_clean appfs_clean module_clean tools_clean


check_config:
	@if [ ! -f .config ]; then \
		echo "\r\nERROR: .config is not exit !!!\r\n"; \
		echo "please use'make platform' chioce platform\r\n"; \
		exit 1; \
	fi

check_sdk: check_config
	@echo "Checking SDK directory..."
	@PLATFORM_NAME=$(patsubst "%",%,$(CONFIG_PLATFORM_CONFIG)); \
	SDK_NAME=$${PLATFORM_NAME}_sdk_source; \
	SDK_DIR=$(OBC_SDK_BASE_DIR)/$$SDK_NAME; \
	if [ ! -d "$(OBC_SDK_BASE_DIR)" ]; then \
		echo "Creating SDK base directory: $(OBC_SDK_BASE_DIR)"; \
		mkdir -p $(OBC_SDK_BASE_DIR); \
	fi; \
	if [ ! -d "$$SDK_DIR" ]; then \
		echo "ERROR: SDK directory not found: $$SDK_DIR"; \
		echo "Please clone the SDK repository to $$SDK_DIR"; \
		exit 1; \
	else \
		echo "SDK directory exists: $$SDK_DIR"; \
	fi; \
	if [ ! -d "$$SDK_DIR/uboot" ]; then \
		echo "ERROR: uboot directory not found in SDK: $$SDK_DIR/uboot"; \
		exit 1; \
	fi; \
	if [ ! -d "$$SDK_DIR/kernel" ]; then \
		echo "ERROR: kernel directory not found in SDK: $$SDK_DIR/kernel"; \
		exit 1; \
	fi

output:
	$(shell mkdir -p $(OBC_PACK_DIR))
	$(shell mkdir -p $(OBC_PACK_IMAGE_DIR))
	$(shell mkdir -p $(OBC_PACK_TOOLS_DIR))
	$(shell mkdir -p $(OBC_APPFS_DIR))
	$(shell mkdir -p $(MODULE_OUTPUT_DIR))
	@if [ -f $(OBC_TOP_DIR)/tools/pack_tools/output/pack ]; then \
		cp $(OBC_TOP_DIR)/tools/pack_tools/output/pack $(OBC_PACK_TOOLS_DIR); \
		cp $(OBC_TOP_DIR)/tools/pack_tools/output/unpack_demo $(OBC_PACK_TOOLS_DIR); \
	else \
		echo "Error: no pack tools\r\n"; \
		exit 1; \
	fi

# 各个成果物编译
# SPL/Loader编译（Rockchip平台需要）
.PHONY: loader
loader:
ifeq ($(CONFIG_UBOOT_BUILD_SPL),y)
	@echo "Building SPL/Loader for Rockchip platform..."
	@SPL_BIN=$(patsubst "%",%,$(CONFIG_SPL_BIN_NAME)); \
	RKBIN_PATH=$(patsubst "%",%,$(CONFIG_RKBIN_PATH)); \
	if [ -z "$$RKBIN_PATH" ]; then \
		RKBIN_PATH="rkbin"; \
	fi; \
	RKBIN_DIR=$(UBOOT_SDK_DIR)/$$RKBIN_PATH; \
	if [ ! -d "$$RKBIN_DIR" ]; then \
		echo "Warning: RKBIN directory not found: $$RKBIN_DIR"; \
		echo "Skipping SPL loader packing"; \
		exit 0; \
	fi; \
	if [ -f $(UBOOT_SDK_DIR)/$$SPL_BIN ]; then \
		echo "SPL binary found: $$SPL_BIN"; \
		INI_LOADER="$$RKBIN_DIR/RKBOOT/RK3562MINIALL.ini"; \
		if [ ! -f "$$INI_LOADER" ]; then \
			echo "Warning: Loader INI not found: $$INI_LOADER"; \
			INI_LOADER=$$(find $$RKBIN_DIR/RKBOOT -name "*3562*" -type f 2>/dev/null | head -1); \
			if [ -z "$$INI_LOADER" ]; then \
				echo "Warning: Cannot find loader INI file for RK3562"; \
				exit 0; \
			else \
				echo "Found alternative INI: $$INI_LOADER"; \
			fi; \
		fi; \
		SCRIPT_SPL="$$RKBIN_DIR/../scripts/spl.sh"; \
		if [ -f "$$SCRIPT_SPL" ]; then \
			echo "Packing SPL loader with $$SCRIPT_SPL"; \
			cd $$RKBIN_DIR && $$SCRIPT_SPL --ini $$INI_LOADER --spl $$(cd $(UBOOT_SDK_DIR) && pwd)/$$SPL_BIN; \
			cd $(UBOOT_SDK_DIR); \
			LOADER_FILE=$$(find $$RKBIN_DIR -name "*loader*.bin" -type f -newer $$SPL_BIN 2>/dev/null | head -1); \
			if [ ! -z "$$LOADER_FILE" ]; then \
				cp $$LOADER_FILE $(UBOOT_SDK_DIR)/; \
				echo "Loader packed: $$(basename $$LOADER_FILE)"; \
			fi; \
		else \
			echo "Warning: SPL script not found: $$SCRIPT_SPL"; \
		fi; \
	else \
		echo "Warning: SPL binary not found: $(UBOOT_SDK_DIR)/$$SPL_BIN"; \
	fi
else
	@echo "SPL/Loader build disabled"
endif

uboot: loader
	@echo "Building U-Boot..."
	@UBOOT_DEFCONFIG=$(patsubst "%",%,$(CONFIG_UBOOT_DEFCONFIG)); \
	UBOOT_TARGET=$(patsubst "%",%,$(CONFIG_UBOOT_BUILD_TARGET)); \
	if [ ! -z "$$UBOOT_DEFCONFIG" ]; then \
		echo "Configuring U-Boot with $$UBOOT_DEFCONFIG"; \
		$(MAKE) -C $(UBOOT_SDK_DIR) $$UBOOT_DEFCONFIG ARCH=$(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH)) CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP)); \
	fi; \
	if [ -z "$$UBOOT_TARGET" ]; then \
		UBOOT_TARGET="all"; \
	fi; \
	echo "Building U-Boot target: $$UBOOT_TARGET"; \
	$(MAKE) -j $(shell expr $(shell nproc) - 1) -C $(UBOOT_SDK_DIR) $$UBOOT_TARGET \
		ARCH=$(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH)) \
		CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
	$(MAKE) -C $(OBC_TOP_DIR) loader
	$(MAKE) -C $(OBC_TOP_DIR) uboot_install

kernel:
	@echo "Building Kernel..."
	@KERNEL_DEFCONFIG=$(patsubst "%",%,$(CONFIG_KERNEL_DEFCONFIG)); \
	KERNEL_TARGET=$(patsubst "%",%,$(CONFIG_KERNEL_BUILD_TARGET)); \
	KERNEL_ARCH=$(patsubst "%",%,$(CONFIG_KERNEL_ARCH)); \
	if [ -z "$$KERNEL_ARCH" ]; then \
		KERNEL_ARCH=$(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH)); \
	fi; \
	if [ ! -z "$$KERNEL_DEFCONFIG" ]; then \
		echo "Configuring Kernel with $$KERNEL_DEFCONFIG"; \
		$(MAKE) -C $(KERNEL_SDK_DIR) $$KERNEL_DEFCONFIG ARCH=$$KERNEL_ARCH CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP)); \
	fi; \
	if [ -z "$$KERNEL_TARGET" ]; then \
		KERNEL_TARGET="all"; \
	fi; \
	echo "Building Kernel target: $$KERNEL_TARGET (ARCH=$$KERNEL_ARCH)"; \
	$(MAKE) -j $(shell expr $(shell nproc) - 1) -C $(KERNEL_SDK_DIR) $$KERNEL_TARGET ARCH=$$KERNEL_ARCH CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
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
	@FDT_NAME=$(patsubst "%",%,$(CONFIG_FDT_BIN_NAME)); \
	if [ -f $(UBOOT_SDK_DIR)/arch/arm/dts/$$FDT_NAME ]; then \
		cp $(UBOOT_SDK_DIR)/arch/arm/dts/$$FDT_NAME $(OBC_PACK_DIR)/; \
		echo "  Installed: $$FDT_NAME (from arch/arm/dts/)"; \
	elif [ -f $(UBOOT_SDK_DIR)/$$FDT_NAME ]; then \
		cp $(UBOOT_SDK_DIR)/$$FDT_NAME $(OBC_PACK_DIR)/; \
		echo "  Installed: $$FDT_NAME (from root)"; \
	else \
		echo "  Warning: $$FDT_NAME not found"; \
	fi
ifeq ($(CONFIG_UBOOT_BUILD_SPL),y)
	@echo "Installing SPL/Loader files..."
	@SPL_BIN=$(patsubst "%",%,$(CONFIG_SPL_BIN_NAME)); \
	if [ -f $(UBOOT_SDK_DIR)/$$SPL_BIN ]; then \
		cp $(UBOOT_SDK_DIR)/$$SPL_BIN $(OBC_PACK_DIR)/; \
		echo "  Installed: $$SPL_BIN"; \
	fi; \
	UBOOT_IMG_SRC=$(UBOOT_SDK_DIR)/u-boot.img; \
	if [ ! -f "$$UBOOT_IMG_SRC" ]; then \
		UBOOT_IMG_SRC=$(UBOOT_SDK_DIR)/uboot.img; \
	fi; \
	if [ -f "$$UBOOT_IMG_SRC" ]; then \
		cp "$$UBOOT_IMG_SRC" $(OBC_PACK_DIR)/uboot.img; \
		echo "  Installed: uboot.img (from $$(basename $$UBOOT_IMG_SRC))"; \
	else \
		echo "  Warning: u-boot.img/uboot.img not found in $(UBOOT_SDK_DIR)"; \
	fi; \
	RKBIN_PATH=$(patsubst "%",%,$(CONFIG_RKBIN_PATH)); \
	if [ -z "$$RKBIN_PATH" ]; then \
		RKBIN_PATH="rkbin"; \
	fi; \
	IDBLOCK_FILE=$(UBOOT_SDK_DIR)/$$RKBIN_PATH/idblock.img; \
	if [ -f "$$IDBLOCK_FILE" ]; then \
		cp "$$IDBLOCK_FILE" $(OBC_PACK_DIR)/idblock.img; \
		echo "  Installed: idblock.img"; \
	else \
		echo "  Warning: idblock.img not found: $$IDBLOCK_FILE"; \
	fi; \
	for loader_file in $(UBOOT_SDK_DIR)/*loader*.bin; do \
		if [ -f "$$loader_file" ]; then \
			cp "$$loader_file" $(OBC_PACK_DIR)/; \
			echo "  Installed: $$(basename $$loader_file)"; \
		fi; \
	done
endif

kernel_install: check_sdk output
	@echo "Installing kernel to $(OBC_PACK_DIR)..."
	@KERNEL_NAME=$(patsubst "%",%,$(CONFIG_KERNEL_BIN_NAME)); \
	if [ -f $(KERNEL_SDK_DIR)/arch/arm/boot/$$KERNEL_NAME ]; then \
		cp $(KERNEL_SDK_DIR)/arch/arm/boot/$$KERNEL_NAME $(OBC_PACK_DIR)/; \
		echo "  Installed: $$KERNEL_NAME (from arch/arm/boot/)"; \
	elif [ -f $(KERNEL_SDK_DIR)/arch/arm64/boot/$$KERNEL_NAME ]; then \
		cp $(KERNEL_SDK_DIR)/arch/arm64/boot/$$KERNEL_NAME $(OBC_PACK_DIR)/; \
		echo "  Installed: $$KERNEL_NAME (from arch/arm64/boot/)"; \
	elif [ -f $(KERNEL_SDK_DIR)/$$KERNEL_NAME ]; then \
		cp $(KERNEL_SDK_DIR)/$$KERNEL_NAME $(OBC_PACK_DIR)/; \
		echo "  Installed: $$KERNEL_NAME (from root)"; \
	else \
		echo "  Warning: $$KERNEL_NAME not found"; \
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
	@make -C $(KERNEL_SDK_DIR) menuconfig ARCH=$(patsubst "%",%,$(CONFIG_KERNEL_ARCH)) CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))

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

# module 驱动模块编译
module: check_config output
	@echo "Building kernel modules..."
	@if [ -z "$(MODULES)" ]; then \
		echo "  No modules enabled in configuration"; \
	else \
		for mod in $(MODULES); do \
			echo "  Building module: $$mod"; \
			$(MAKE) -C $(MODULE_DIR)/$$mod all \
				KERNEL_SDK_DIR=$(KERNEL_SDK_DIR) \
				CONFIG_OBC_SDK_ARCH=$(CONFIG_OBC_SDK_ARCH) \
				CONFIG_OBC_SDK_COMP=$(CONFIG_OBC_SDK_COMP) \
				MODULE_OUTPUT_DIR=$(MODULE_OUTPUT_DIR); \
			if [ $$? -ne 0 ]; then \
				echo "Error: Failed to build module $$mod"; \
				exit 1; \
			fi; \
		done; \
	fi

module_install: module
	@echo "Installing kernel modules to $(MODULE_OUTPUT_DIR)..."
	@if [ -z "$(MODULES)" ]; then \
		echo "  No modules to install"; \
	else \
		for mod in $(MODULES); do \
			$(MAKE) -C $(MODULE_DIR)/$$mod install \
				KERNEL_SDK_DIR=$(KERNEL_SDK_DIR) \
				CONFIG_OBC_SDK_ARCH=$(CONFIG_OBC_SDK_ARCH) \
				CONFIG_OBC_SDK_COMP=$(CONFIG_OBC_SDK_COMP) \
				MODULE_OUTPUT_DIR=$(MODULE_OUTPUT_DIR); \
		done; \
		echo "Module installation complete!"; \
	fi

module_clean:
	@echo "Cleaning kernel modules..."
	@if [ -d "$(MODULE_DIR)" ]; then \
		for mod in $(MODULES); do \
			if [ -d "$(MODULE_DIR)/$$mod" ]; then \
				echo "  Cleaning module: $$mod"; \
				$(MAKE) -C $(MODULE_DIR)/$$mod clean \
					KERNEL_SDK_DIR=$(KERNEL_SDK_DIR) \
					CONFIG_OBC_SDK_ARCH=$(CONFIG_OBC_SDK_ARCH) \
					CONFIG_OBC_SDK_COMP=$(CONFIG_OBC_SDK_COMP); \
			fi; \
		done; \
	fi
	@if [ -d "$(MODULE_OUTPUT_DIR)" ]; then \
		rm -rf $(MODULE_OUTPUT_DIR)/*; \
		echo "  Cleaned module output directory"; \
	fi

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
	@echo "  make menuconfig  			- 配置OBC SDK (包括appfs应用和内核模块)"
	@echo "  make defconfig   			- 加载当前平台的默认配置"
	@echo "  make savedefconfig  		- 保存当前配置到平台defconfig文件"
	@echo "  make appfs       			- 编译appfs应用程序"
	@echo "  make appfs_install       	- 安装appfs应用程序到output/appfs"
	@echo "  make module      			- 编译内核驱动模块"
	@echo "  make module_install      	- 安装内核驱动模块到output/modules"
	@echo "  make tools       			- 编译tools工具 (pack/unpack)"
	@echo "  make help        			- 显示此帮助信息"
	@echo ""
	@echo "Output directory: $(OBC_OUTPUT_DIR)"
	@echo "  - tmp/         : 中间成果物 (uboot, kernel, rootfs)"
	@echo "  - image/       : 打包后的镜像文件"
	@echo "  - tools/       : 打包工具"
	@echo "  - appfs/       : 应用程序"
	@echo "  - modules/     : 内核驱动模块"

	@echo "  make uboot/kernel/rootfs   - 编译指定资源"
	@echo "  make clean    				- 清除"


.PHONY: platform help
