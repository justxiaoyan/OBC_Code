AM62X_UBOOT_TMP := $(OBC_PACK_DIR)/$(CONFIG_UBOOT_BIN_NAME)
AM62X_TISPL_TMP := $(OBC_PACK_DIR)/$(CONFIG_TISPL_BIN_NAME)
AM62X_UBOOT_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-uboot.bin
AM62X_TISPL_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-teeos.bin
AM62X_UBOOT_CONFIG := $(OBC_TOP_DIR)/platform_config/am62x/sdk_config/uboot-am62x-defconfig

.PHONY: uboot uboot_build uboot_build_install uboot_build_clean
uboot: uboot_build_install

uboot_build: check_sdk output
	@set -eu; \
	if [ ! -f "$(AM62X_UBOOT_CONFIG)" ]; then echo "ERROR: U-Boot defconfig not found: $(AM62X_UBOOT_CONFIG)"; exit 1; fi; \
	cp "$(AM62X_UBOOT_CONFIG)" "$(UBOOT_SDK_DIR)/.config"; \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" $(UBOOT_TOOLCHAIN_VARS); \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" -j$$(nproc) all ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" $(UBOOT_TOOLCHAIN_VARS)

uboot_build_install: uboot_build
	@set -eu; \
	find_src() { for f in "$$1" "$$1_unsigned"; do if [ -f "$$f" ]; then echo "$$f"; return 0; fi; done; return 1; }; \
	uboot_src=$$(find_src "$(UBOOT_SDK_DIR)/u-boot.img") || { echo "ERROR: u-boot.img not found"; exit 1; }; \
	tispl_src=$$(find_src "$(UBOOT_SDK_DIR)/tispl.bin") || { echo "ERROR: tispl.bin not found"; exit 1; }; \
	cp "$$uboot_src" "$(AM62X_UBOOT_TMP)"; cp "$$tispl_src" "$(AM62X_TISPL_TMP)"; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_UBOOT_TMP)" PACK_OUTPUT="$(AM62X_UBOOT_IMAGE)" PACK_HEAD_WRITE=1; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_TISPL_TMP)" PACK_OUTPUT="$(AM62X_TISPL_IMAGE)" PACK_HEAD_WRITE=1

uboot_build_clean:
	-$(MAKE) -C "$(UBOOT_SDK_DIR)" clean
	rm -f "$(AM62X_UBOOT_TMP)" "$(AM62X_TISPL_TMP)" "$(AM62X_UBOOT_IMAGE)" "$(AM62X_TISPL_IMAGE)"
uboot_clean: uboot_build_clean
