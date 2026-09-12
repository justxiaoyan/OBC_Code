AM62X_ROOTFS_TMP := $(OBC_PACK_DIR)/$(CONFIG_ROOTFS_BIN_NAME)
AM62X_ROOTFS_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-rootfs.bin

.PHONY: rootfs rootfs_build rootfs_build_install rootfs_build_clean
rootfs: rootfs_build_install

rootfs_build: check_config output
	@set -eu; \
	$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" rootfs_pack ARCH="$(CONFIG_OBC_SDK_ARCH)" CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"; \
	src="$(ROOTFS_SOURCE_DIR)/rootfs.ext2"; \
	if [ ! -f "$$src" ]; then echo "ERROR: rootfs output not found: $$src"; exit 1; fi; \
	cp "$$src" "$(AM62X_ROOTFS_TMP)"

rootfs_build_install: sign_tools rootfs_build
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_ROOTFS_TMP)" PACK_OUTPUT="$(AM62X_ROOTFS_IMAGE)" PACK_HEAD_WRITE=0

rootfs_build_clean:
	-$(MAKE) -C "$(ROOTFS_SOURCE_DIR)" clean
	rm -f "$(AM62X_ROOTFS_TMP)" "$(AM62X_ROOTFS_IMAGE)"
rootfs_clean: rootfs_build_clean
