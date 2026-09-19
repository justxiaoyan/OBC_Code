AM62X_LOADER_CONFIG := $(OBC_TOP_DIR)/$(CONFIG_R5_LOADER_DEFCONFIG)
AM62X_LOADER_SOURCE := $(UBOOT_SDK_DIR)/$(CONFIG_R5_LOADER_BIN_NAME)
AM62X_LOADER_TMP := $(OBC_PACK_DIR)/$(CONFIG_LOADER_BIN_NAME)
AM62X_LOADER_IMAGE := $(call OBC_UPGRADE_IMAGE,loader)
AM62X_LEGACY_LOADER_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-loader.bin
AM62X_R5_GCC := $(firstword $(shell command -v arm-none-eabi-gcc 2>/dev/null) \
	$(wildcard $(OBC_TOOLCHAIN_ROOT)/bin/arm-none-eabi-gcc) \
	$(wildcard $(OBC_TOOLCHAIN_ROOT)/opt/gcc-arm-none-eabi/bin/arm-none-eabi-gcc))
AM62X_R5_TOOLCHAIN_DIR := $(patsubst %/,%,$(dir $(AM62X_R5_GCC)))

.PHONY: loader loader_build loader_build_install loader_build_clean
loader: loader_build_install

loader_build: check_sdk output obcbase_sync
	@set -eu; \
	cleanup() { \
		git -C "$(UBOOT_SDK_DIR)" restore -- $(OBCBASE_PATCH_FILES); \
		find "$(UBOOT_SDK_DIR)/obcbase" -depth -type f -delete 2>/dev/null || true; \
		find "$(UBOOT_SDK_DIR)/obcbase" -depth -type d -empty -delete 2>/dev/null || true; \
	}; \
	trap cleanup EXIT INT TERM; \
	if [ ! -f "$(AM62X_LOADER_CONFIG)" ]; then echo "ERROR: loader defconfig not found: $(AM62X_LOADER_CONFIG)"; exit 1; fi; \
	if [ -z "$(AM62X_R5_GCC)" ] || [ ! -x "$(AM62X_R5_GCC)" ]; then echo "ERROR: arm-none-eabi-gcc not found; checked PATH and $(OBC_TOOLCHAIN_ROOT)/{bin,opt/gcc-arm-none-eabi/bin}"; exit 1; fi; \
	export PATH="$(AM62X_R5_TOOLCHAIN_DIR):$$PATH"; \
	if [ -z "$(BINMAN_INDIRS)" ] || [ ! -d "$(BINMAN_INDIRS)" ]; then echo "ERROR: TI K3 firmware directory not found"; exit 1; fi; \
	cp "$(AM62X_LOADER_CONFIG)" "$(UBOOT_SDK_DIR)/.config"; \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE=arm-none-eabi- BINMAN_INDIRS="$(BINMAN_INDIRS)"; \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" -j$$(nproc) all ARCH=arm CROSS_COMPILE=arm-none-eabi- BINMAN_INDIRS="$(BINMAN_INDIRS)"

loader_build_install: sign_tools loader_build
	@set -eu; \
	src="$(AM62X_LOADER_SOURCE)"; \
	if [ ! -f "$$src" ]; then src="$(UBOOT_SDK_DIR)/spl/u-boot-spl.bin"; fi; \
	if [ ! -f "$$src" ]; then echo "ERROR: loader output not found under $(UBOOT_SDK_DIR)"; exit 1; fi; \
	rm -f "$(AM62X_LEGACY_LOADER_IMAGE)"; \
	cp "$$src" "$(AM62X_LOADER_TMP)"; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_LOADER_TMP)" PACK_OUTPUT="$(AM62X_LOADER_IMAGE)" PACK_HEAD_WRITE=0

loader_build_clean:
	-$(MAKE) -C "$(UBOOT_SDK_DIR)" clean
	rm -f "$(AM62X_LOADER_TMP)" "$(AM62X_LOADER_IMAGE)" "$(AM62X_LEGACY_LOADER_IMAGE)"
loader_clean: loader_build_clean
