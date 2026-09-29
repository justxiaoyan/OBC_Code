IMX6ULL_ROOTFS_TMP := $(OBC_PACK_DIR)/$(CONFIG_ROOTFS_BIN_NAME)
IMX6ULL_ROOTFS_IMAGE := $(call OBC_UPGRADE_IMAGE,rootfs)
IMX6ULL_ROOTFS_CONFIG := $(OBC_TOP_DIR)/platform_config/imx6ull/sdk_config/kernel-imx6ull-defconfig
CONFIG_OBC_BASE ?= y

.PHONY: rootfs rootfs_build rootfs_build_install rootfs_build_clean rootfs_clean

rootfs: rootfs_build_install

rootfs_build: check_sdk output
	@set -eu; \
	$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" rootfs_pack \
		ARCH="$(CONFIG_OBC_SDK_ARCH)" \
		CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" \
		SYSROOT="$(OBC_TOOLCHAIN_SYSROOT)" \
		KERNEL_SDK_DIR="$(KERNEL_SDK_DIR)" \
		KERNEL_CONFIG="$(IMX6ULL_ROOTFS_CONFIG)" \
		CONFIG_OBC_BASE="$(CONFIG_OBC_BASE)" \
		MAKE_EXT4FS="$(MAKE_EXT4FS)" \
		ROOTFS_OUTPUT_FILE="$(IMX6ULL_ROOTFS_TMP)"; \
	if [ ! -f "$(IMX6ULL_ROOTFS_TMP)" ]; then \
		echo "ERROR: rootfs output not found: $(IMX6ULL_ROOTFS_TMP)"; \
		exit 1; \
	fi

rootfs_build_install: sign_tools rootfs_build
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed \
		PACK_INPUT="$(IMX6ULL_ROOTFS_TMP)" \
		PACK_OUTPUT="$(IMX6ULL_ROOTFS_IMAGE)" \
		PACK_HEAD_WRITE=0

rootfs_build_clean:
	-$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" clean \
		ROOTFS_OUTPUT_FILE="$(IMX6ULL_ROOTFS_TMP)" \
		ROOTFS_STAGING_DIR="$(OBC_PACK_DIR)/install" \
		BUSYBOX_BUILD_DIR="$(OBC_PACK_DIR)/busybox-build" \
		KERNEL_SDK_DIR="$(KERNEL_SDK_DIR)" \
		KERNEL_CONFIG="$(IMX6ULL_ROOTFS_CONFIG)" \
		OBCBASE_BUILD_DIR="$(OBC_PACK_DIR)/obcbase-build" \
		OBCTOOL_BUILD_DIR="$(OBC_PACK_DIR)/obctool-build"
	rm -f "$(IMX6ULL_ROOTFS_TMP)" "$(IMX6ULL_ROOTFS_TMP).tmp" "$(IMX6ULL_ROOTFS_IMAGE)"

rootfs_clean: rootfs_build_clean
