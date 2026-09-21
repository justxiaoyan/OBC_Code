AM62X_ROOTFS_TMP := $(OBC_PACK_DIR)/$(CONFIG_ROOTFS_BIN_NAME)
AM62X_ROOTFS_IMAGE := $(call OBC_UPGRADE_IMAGE,rootfs)

.PHONY: rootfs rootfs_build rootfs_build_install rootfs_build_clean

rootfs: rootfs_build_install

rootfs_build: check_sdk output
	@set -eu; \
	$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" rootfs_pack \
		ARCH="$(subst ",,$(CONFIG_KERNEL_ARCH))" \
		CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" \
		SYSROOT="$(OBC_TOOLCHAIN_SYSROOT)" \
		KERNEL_SDK_DIR="$(KERNEL_SDK_DIR)" \
		KERNEL_CONFIG="$(OBC_TOP_DIR)/platform_config/am62x/sdk_config/kernel-am62x-defconfig" \
		CONFIG_OBC_BASE="$(CONFIG_OBC_BASE)" \
		MAKE_EXT4FS="$(MAKE_EXT4FS)" \
		ROOTFS_OUTPUT_FILE="$(AM62X_ROOTFS_TMP)"; \
	if [ ! -f "$(AM62X_ROOTFS_TMP)" ]; then \
		echo "ERROR: sparse rootfs output not found: $(AM62X_ROOTFS_TMP)"; \
		exit 1; \
	fi

rootfs_build_install: sign_tools rootfs_build
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_ROOTFS_TMP)" PACK_OUTPUT="$(AM62X_ROOTFS_IMAGE)" PACK_HEAD_WRITE=0

rootfs_build_clean:
	-$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" clean \
		ROOTFS_OUTPUT_FILE="$(AM62X_ROOTFS_TMP)" \
		ROOTFS_STAGING_DIR="$(OBC_PACK_DIR)/install" \
		BUSYBOX_BUILD_DIR="$(OBC_PACK_DIR)/busybox-build"
	rm -f "$(AM62X_ROOTFS_TMP)" "$(AM62X_ROOTFS_TMP).tmp" "$(AM62X_ROOTFS_IMAGE)"

rootfs_clean: rootfs_build_clean
