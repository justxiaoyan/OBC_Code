RK3562_ROOTFS_TMP := $(OBC_PACK_DIR)/$(CONFIG_ROOTFS_BIN_NAME)
RK3562_ROOTFS_IMAGE := $(call OBC_UPGRADE_IMAGE,rootfs)
CONFIG_OBC_BASE ?= y

.PHONY: rootfs rootfs_build rootfs_build_install rootfs_clean
rootfs: rootfs_build_install

rootfs_build: check_sdk output
	@set -eu; \
	LD_LIBRARY_PATH="$(OBC_TOOLCHAIN_ROOT)/lib:$${LD_LIBRARY_PATH:-}" \
	$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" rootfs_pack \
		ARCH="$(CONFIG_KERNEL_ARCH)" \
		CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" \
		SYSROOT="$(OBC_TOOLCHAIN_SYSROOT)" \
		KERNEL_SDK_DIR="$(KERNEL_SDK_DIR)" \
		KERNEL_CONFIG="$(OBC_TOP_DIR)/platform_config/rk3562/sdk_config/kernel-rk3562-defconfig" \
		CONFIG_OBC_BASE="$(CONFIG_OBC_BASE)" \
		MAKE_EXT4FS="$(MAKE_EXT4FS)" \
		ROOTFS_OUTPUT_FILE="$(RK3562_ROOTFS_TMP)"; \
	test -s "$(RK3562_ROOTFS_TMP)" || { echo "ERROR: sparse rootfs output not found: $(RK3562_ROOTFS_TMP)"; exit 1; }

rootfs_build_install: sign_tools rootfs_build
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(RK3562_ROOTFS_TMP)" PACK_OUTPUT="$(RK3562_ROOTFS_IMAGE)" PACK_HEAD_WRITE=0

rootfs_clean:
	-$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" clean ROOTFS_OUTPUT_FILE="$(RK3562_ROOTFS_TMP)"
	rm -f "$(RK3562_ROOTFS_TMP)" "$(RK3562_ROOTFS_TMP).tmp" "$(RK3562_ROOTFS_IMAGE)"
