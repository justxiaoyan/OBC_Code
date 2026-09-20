AM62X_KERNEL_TMP := $(OBC_PACK_DIR)/$(CONFIG_KERNEL_BIN_NAME)
AM62X_KERNEL_IMAGE := $(call OBC_UPGRADE_IMAGE,kernel)
AM62X_LEGACY_KERNEL_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-kernel.bin
AM62X_KERNEL_CONFIG := $(OBC_TOP_DIR)/platform_config/am62x/sdk_config/kernel-am62x-defconfig
AM62X_INITRAMFS := $(OBC_TOP_DIR)/scripts/am62x/rootfs.cpio.gz
AM62X_INITRAMFS_LIST := $(OBC_PACK_DIR)/rootfs-cpio.list
AM62X_KERNEL_ARCH := $(subst ",,$(CONFIG_KERNEL_ARCH))

.PHONY: kernel kernel_build kernel_build_install kernel_build_clean
kernel: kernel_build_install

kernel_build: check_sdk output
	@set -eu; \
	if [ ! -f "$(AM62X_KERNEL_CONFIG)" ]; then echo "ERROR: kernel defconfig not found: $(AM62X_KERNEL_CONFIG)"; exit 1; fi; \
	if [ ! -f "$(AM62X_INITRAMFS)" ]; then echo "ERROR: archived initramfs not found: $(AM62X_INITRAMFS)"; echo "       run './build.sh rootfs' and archive the result first"; exit 1; fi; \
	if ! gzip -t "$(AM62X_INITRAMFS)"; then echo "ERROR: invalid gzip initramfs: $(AM62X_INITRAMFS)"; exit 1; fi; \
	rm -f "$(AM62X_INITRAMFS_LIST)"; \
	if ! gzip -dc "$(AM62X_INITRAMFS)" | cpio --quiet -it > "$(AM62X_INITRAMFS_LIST)" 2>/dev/null; then echo "ERROR: invalid cpio initramfs: $(AM62X_INITRAMFS)"; rm -f "$(AM62X_INITRAMFS_LIST)"; exit 1; fi; \
	if ! grep -Fqx init "$(AM62X_INITRAMFS_LIST)"; then echo "ERROR: initramfs does not contain /init"; rm -f "$(AM62X_INITRAMFS_LIST)"; exit 1; fi; \
	if ! grep -Fqx bin/busybox "$(AM62X_INITRAMFS_LIST)"; then echo "ERROR: initramfs does not contain /bin/busybox"; rm -f "$(AM62X_INITRAMFS_LIST)"; exit 1; fi; \
	rm -f "$(AM62X_INITRAMFS_LIST)"; \
	cp "$(AM62X_KERNEL_CONFIG)" "$(KERNEL_SDK_DIR)/.config"; \
	"$(KERNEL_SDK_DIR)/scripts/config" --file "$(KERNEL_SDK_DIR)/.config" \
		--set-str INITRAMFS_SOURCE "$(AM62X_INITRAMFS)" \
		--enable BLK_DEV_INITRD \
		--enable RD_GZIP; \
	$(MAKE) -C "$(KERNEL_SDK_DIR)" olddefconfig ARCH="$(AM62X_KERNEL_ARCH)" CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"; \
	if ! grep -Fqx 'CONFIG_INITRAMFS_SOURCE="$(AM62X_INITRAMFS)"' "$(KERNEL_SDK_DIR)/.config"; then echo "ERROR: kernel initramfs configuration was not retained"; exit 1; fi; \
	$(MAKE) -C "$(KERNEL_SDK_DIR)" -j$$(nproc) Image.gz ARCH="$(AM62X_KERNEL_ARCH)" CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"; \
	if [ ! -f "$(KERNEL_SDK_DIR)/usr/initramfs_inc_data" ]; then echo "ERROR: kernel did not generate embedded initramfs data"; exit 1; fi; \
	if ! cmp -s "$(AM62X_INITRAMFS)" "$(KERNEL_SDK_DIR)/usr/initramfs_inc_data"; then echo "ERROR: embedded initramfs does not match the archived cpio"; exit 1; fi

kernel_build_install: sign_tools kernel_build
	@set -eu; \
	src="$(KERNEL_SDK_DIR)/arch/$(AM62X_KERNEL_ARCH)/boot/$(CONFIG_KERNEL_BIN_NAME)"; \
	if [ ! -f "$$src" ]; then echo "ERROR: kernel output not found: $$src"; exit 1; fi; \
	rm -f "$(AM62X_LEGACY_KERNEL_IMAGE)"; \
	cp "$$src" "$(AM62X_KERNEL_TMP)"; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_KERNEL_TMP)" PACK_OUTPUT="$(AM62X_KERNEL_IMAGE)" PACK_HEAD_WRITE=1

kernel_build_clean:
	-$(MAKE) -C "$(KERNEL_SDK_DIR)" clean
	rm -f "$(AM62X_KERNEL_TMP)" "$(AM62X_KERNEL_IMAGE)" "$(AM62X_LEGACY_KERNEL_IMAGE)" "$(AM62X_INITRAMFS_LIST)"
kernel_clean: kernel_build_clean
