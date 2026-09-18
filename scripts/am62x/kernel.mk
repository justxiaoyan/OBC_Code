AM62X_KERNEL_TMP := $(OBC_PACK_DIR)/$(CONFIG_KERNEL_BIN_NAME)
AM62X_KERNEL_IMAGE := $(call OBC_UPGRADE_IMAGE,kernel)
AM62X_LEGACY_KERNEL_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-kernel.bin
AM62X_KERNEL_CONFIG := $(OBC_TOP_DIR)/platform_config/am62x/sdk_config/kernel-am62x-defconfig

.PHONY: kernel kernel_build kernel_build_install kernel_build_clean
kernel: kernel_build_install

kernel_build: check_sdk output
	@set -eu; \
	if [ ! -f "$(AM62X_KERNEL_CONFIG)" ]; then echo "ERROR: kernel defconfig not found: $(AM62X_KERNEL_CONFIG)"; exit 1; fi; \
	cp "$(AM62X_KERNEL_CONFIG)" "$(KERNEL_SDK_DIR)/.config"; \
	$(MAKE) -C "$(KERNEL_SDK_DIR)" olddefconfig ARCH="$(CONFIG_KERNEL_ARCH)" CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"; \
	$(MAKE) -C "$(KERNEL_SDK_DIR)" -j$$(nproc) Image.gz ARCH="$(CONFIG_KERNEL_ARCH)" CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"

kernel_build_install: sign_tools kernel_build
	@set -eu; \
	src="$(KERNEL_SDK_DIR)/arch/$(CONFIG_KERNEL_ARCH)/boot/$(CONFIG_KERNEL_BIN_NAME)"; \
	if [ ! -f "$$src" ]; then echo "ERROR: kernel output not found: $$src"; exit 1; fi; \
	rm -f "$(AM62X_LEGACY_KERNEL_IMAGE)"; \
	cp "$$src" "$(AM62X_KERNEL_TMP)"; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_KERNEL_TMP)" PACK_OUTPUT="$(AM62X_KERNEL_IMAGE)" PACK_HEAD_WRITE=1

kernel_build_clean:
	-$(MAKE) -C "$(KERNEL_SDK_DIR)" clean
	rm -f "$(AM62X_KERNEL_TMP)" "$(AM62X_KERNEL_IMAGE)" "$(AM62X_LEGACY_KERNEL_IMAGE)"
kernel_clean: kernel_build_clean
