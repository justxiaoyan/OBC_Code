AM62X_ROOTFS_IMAGE := $(OBC_TOP_DIR)/scripts/am62x/rootfs.cpio.gz

.PHONY: rootfs rootfs_build rootfs_build_install rootfs_build_clean

rootfs: rootfs_build

rootfs_build: check_sdk output
	@set -eu; \
	$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" rootfs_pack \
		ARCH="$(subst ",,$(CONFIG_KERNEL_ARCH))" \
		CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" \
		SYSROOT="$(OBC_TOOLCHAIN_SYSROOT)" \
		KERNEL_SDK_DIR="$(KERNEL_SDK_DIR)" \
		KERNEL_CONFIG="$(OBC_TOP_DIR)/platform_config/am62x/sdk_config/kernel-am62x-defconfig" \
		CONFIG_OBC_BASE="$(CONFIG_OBC_BASE)" \
		ROOTFS_OUTPUT_FILE="$(AM62X_ROOTFS_IMAGE)"; \
	if [ ! -f "$(AM62X_ROOTFS_IMAGE)" ]; then \
		echo "ERROR: initramfs output not found: $(AM62X_ROOTFS_IMAGE)"; \
		exit 1; \
	fi

# Keep the old dependency name available to callers, but do not sign or wrap
# the cpio archive as a block rootfs image.
rootfs_build_install: rootfs_build

rootfs_build_clean:
	-$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" clean \
		ROOTFS_OUTPUT_FILE="$(AM62X_ROOTFS_IMAGE)" \
		ROOTFS_STAGING_DIR="$(OBC_PACK_DIR)/install" \
		BUSYBOX_BUILD_DIR="$(OBC_PACK_DIR)/busybox-build"
	rm -f "$(AM62X_ROOTFS_IMAGE).tmp"

rootfs_clean: rootfs_build_clean
