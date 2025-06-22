

# OBC top dir
TFTP_DEBUG_DIR := "/home/wanguo/99-tftp"

OBC_TOP_DIR := $(shell pwd)
export OBC_TOP_DIR
OBC_SDK_DIR := $(shell pwd)/../sdk-source

sinclude $(OBC_TOP_DIR)/.config
include platform_config/platform.mk

UBOOT_SDK_DIR := $(OBC_SDK_DIR)/uboot/uboot-$(UBOOT_VERSION)
KERNEL_SDK_DIR := $(OBC_SDK_DIR)/kernel/kernel-$(KERNEL_VERSION)
ROOTFS_SDK_DIR := $(OBC_SDK_DIR)/rootfs

# 中间产物路径
OBC_PACK_DIR 		:= $(OBC_TOP_DIR)/output/tmp
# 输出可以用于升级的路径
OBC_PACK_IMAGE_DIR 	:= $(OBC_TOP_DIR)/output/image
# pack工具包路径
OBC_PACK_TOOLS_DIR 	:= $(OBC_TOP_DIR)/output/tools


.PHONY: all clean
all : check_config uboot kernel rootfs
install: check_config uboot_install kernel_install rootfs_install
clean: uboot_clean kernel_clean rootfs_clean pack_clean


check_config:
	@if [ ! -f .config ]; then \
		echo "\r\nERROR: .config is not exit !!!\r\n"; \
		echo "please use'make platform' chioce platform\r\n"; \
		exit 1; \
	fi

output:
	$(shell mkdir -p $(OBC_PACK_DIR))
	$(shell mkdir -p $(OBC_PACK_IMAGE_DIR))
	$(shell mkdir -p $(OBC_PACK_TOOLS_DIR))
	@if [ -f $(OBC_TOP_DIR)/tools/pack_tools/output/pack ]; then \
		cp $(OBC_TOP_DIR)/tools/pack_tools/output/pack $(OBC_PACK_TOOLS_DIR); \
		cp $(OBC_TOP_DIR)/tools/pack_tools/output/unpack_demo $(OBC_PACK_TOOLS_DIR); \
	else \
		echo "Error: no pack tools\r\n"; \
		exit 1; \
	fi

# 各个成果物编译
uboot:
	$(MAKE)  -j 15 -C $(UBOOT_SDK_DIR)
	$(MAKE) -C $(OBC_TOP_DIR) uboot_install

kernel:
	$(MAKE) -j 15 -C $(KERNEL_SDK_DIR)
	$(MAKE) -C $(OBC_TOP_DIR) kernel_install

rootfs:
	$(MAKE) -C $(ROOTFS_SDK_DIR) rootfs_pack
	$(MAKE) -C $(OBC_TOP_DIR) rootfs_install

# 成果物清除路径
uboot_clean:
	$(MAKE) -C $(UBOOT_SDK_DIR) clean

kernel_clean:
	$(MAKE) -C $(KERNEL_SDK_DIR) clean

rootfs_clean:
	$(MAKE) -C $(ROOTFS_SDK_DIR) clean

# 成果物移动到output目标路径
uboot_install: check_config output
	@if [ -f $(UBOOT_SDK_DIR)/$(UBOOT_BIN_NAME) ]; then \
		cp $(UBOOT_SDK_DIR)/$(UBOOT_BIN_NAME) $(OBC_PACK_DIR); \
		cp $(UBOOT_SDK_DIR)/arch/arm/dts/$(FDT_BIN_NAME) $(OBC_PACK_DIR); \
	fi

kernel_install: check_config output
	@if [ -f $(KERNEL_SDK_DIR)/arch/arm/boot/$(KERNEL_BIN_NAME) ]; then \
		cp $(KERNEL_SDK_DIR)/arch/arm/boot/$(KERNEL_BIN_NAME) $(OBC_PACK_DIR); \
	fi

rootfs_install: check_config output
	@if [ -f $(ROOTFS_SDK_DIR)/$(ROOTFS_BIN_NAME) ]; then \
		cp $(ROOTFS_SDK_DIR)/$(ROOTFS_BIN_NAME) $(OBC_PACK_DIR); \
	fi

# 将需要pack的成果物进行重新打包，目前就是fdt和kernel需要pack
pack: check_config output uboot_install kernel_install rootfs_install
	@if [ -f $(OBC_PACK_DIR)/$(UBOOT_BIN_NAME) ]; then \
		cp $(OBC_PACK_DIR)/$(UBOOT_BIN_NAME) $(OBC_PACK_IMAGE_DIR)/uboot.bin; \
		$(OBC_PACK_TOOLS_DIR)/pack $(OBC_PACK_DIR)/$(FDT_BIN_NAME) $(OBC_PACK_IMAGE_DIR)/fdt.dtb; \
	fi
	@if [ -f $(OBC_PACK_DIR)/$(KERNEL_BIN_NAME) ]; then \
		$(OBC_PACK_TOOLS_DIR)/pack $(OBC_PACK_DIR)/$(KERNEL_BIN_NAME) $(OBC_PACK_IMAGE_DIR)/zImage; \
	fi
	@if [ -f $(OBC_PACK_DIR)/$(ROOTFS_BIN_NAME) ]; then \
		cp $(OBC_PACK_DIR)/$(ROOTFS_BIN_NAME) $(OBC_PACK_IMAGE_DIR)/rootfs.bin; \
	fi


pack_install: pack
	@cp $(OBC_PACK_IMAGE_DIR)/* $(TFTP_DEBUG_DIR)

pack_clean:
	rm $(OBC_PACK_DIR)/*;
	rm $(OBC_PACK_IMAGE_DIR)/*;

uboot_menuconfig:
	@make -C $(UBOOT_SDK_DIR) menuconfig

kernel_menuconfig:
	@make -C $(KERNEL_SDK_DIR) menuconfig



# 显示帮助信息
help:
	@echo "********************Command list:"
	@echo "  make all         			- 编译所有资源"
	@echo "  make pack        			- 对需要添加头信息的资源进行打包"
	@echo "  make pack_install        	- 对需要添加头信息的资源进行打包 & 并填充到tftp目录"
	@echo "  make platform    			- 选择平台并拷贝对应的defconfig文件"
	@echo "  make help        			- 显示此帮助信息"

	@echo "  make uboot/kernel/rootfs   - 编译指定资源"
	@echo "  make clean    				- 清除"


.PHONY: platform help
