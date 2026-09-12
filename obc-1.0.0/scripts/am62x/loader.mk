AM62X_LOADER_CONFIG := $(OBC_TOP_DIR)/$(CONFIG_R5_LOADER_DEFCONFIG)
AM62X_LOADER_SOURCE := $(UBOOT_SDK_DIR)/$(CONFIG_R5_LOADER_BIN_NAME)
AM62X_LOADER_TMP := $(OBC_PACK_DIR)/$(CONFIG_LOADER_BIN_NAME)
AM62X_LOADER_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-loader.bin

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
	command -v arm-none-eabi-gcc >/dev/null 2>&1 || { echo "ERROR: arm-none-eabi-gcc not found"; exit 1; }; \
	if [ -z "$(BINMAN_INDIRS)" ] || [ ! -d "$(BINMAN_INDIRS)" ]; then echo "ERROR: TI K3 firmware directory not found"; exit 1; fi; \
	cp "$(AM62X_LOADER_CONFIG)" "$(UBOOT_SDK_DIR)/.config"; \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE=arm-none-eabi- BINMAN_INDIRS="$(BINMAN_INDIRS)"; \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" -j$$(nproc) all ARCH=arm CROSS_COMPILE=arm-none-eabi- BINMAN_INDIRS="$(BINMAN_INDIRS)"

loader_build_install: sign_tools loader_build
	@set -eu; \
	src="$(AM62X_LOADER_SOURCE)"; \
	if [ ! -f "$$src" ]; then src="$(UBOOT_SDK_DIR)/spl/u-boot-spl.bin"; fi; \
	if [ ! -f "$$src" ]; then echo "ERROR: loader output not found under $(UBOOT_SDK_DIR)"; exit 1; fi; \
	cp "$$src" "$(AM62X_LOADER_TMP)"; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_LOADER_TMP)" PACK_OUTPUT="$(AM62X_LOADER_IMAGE)" PACK_HEAD_WRITE=0

loader_build_clean:
	-$(MAKE) -C "$(UBOOT_SDK_DIR)" clean
	rm -f "$(AM62X_LOADER_TMP)" "$(AM62X_LOADER_IMAGE)"
loader_clean: loader_build_clean
