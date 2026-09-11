AM62X_UBOOT_TMP := $(OBC_PACK_DIR)/$(CONFIG_UBOOT_BIN_NAME)
AM62X_TISPL_TMP := $(OBC_PACK_DIR)/$(CONFIG_TISPL_BIN_NAME)
AM62X_FDT_SOURCE := $(OBC_TOP_DIR)/dts/am62x/k3-am625-sk.dts
AM62X_FDT_TMP := $(OBC_PACK_DIR)/$(subst ",,$(CONFIG_FDT_BIN_NAME))
AM62X_FDT_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-fdt.bin
AM62X_FDT_BUILD_DIR := $(OBC_TOP_DIR)/dts/am62x
AM62X_FDT_BUILT := $(AM62X_FDT_BUILD_DIR)/k3-am625-sk.dtb
AM62X_FDT_PREPROCESSED := $(AM62X_FDT_BUILD_DIR)/k3-am625-sk.temp.dts
AM62X_CPP := $(shell command -v cpp 2>/dev/null)
AM62X_DTC := $(shell command -v dtc 2>/dev/null)
AM62X_UBOOT_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-uboot.bin
AM62X_TISPL_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-teeos.bin
AM62X_UBOOT_CONFIG := $(OBC_TOP_DIR)/platform_config/am62x/sdk_config/uboot-am62x-defconfig

.PHONY: uboot uboot_build uboot_build_install uboot_build_clean fdt_build fdt_build_clean
uboot: uboot_build_install

uboot_build: check_sdk output fdt_build
	@set -eu; \
	if [ ! -f "$(AM62X_UBOOT_CONFIG)" ]; then echo "ERROR: U-Boot defconfig not found: $(AM62X_UBOOT_CONFIG)"; exit 1; fi; \
	cp "$(AM62X_UBOOT_CONFIG)" "$(UBOOT_SDK_DIR)/.config"; \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" $(UBOOT_TOOLCHAIN_VARS); \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" -j$$(nproc) all ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" $(UBOOT_TOOLCHAIN_VARS)

fdt_build: check_config output
	@set -eu; \
	if [ ! -f "$(AM62X_FDT_SOURCE)" ]; then echo "ERROR: DTS source not found: $(AM62X_FDT_SOURCE)"; exit 1; fi; \
	if [ -z "$(AM62X_CPP)" ] || [ ! -x "$(AM62X_CPP)" ]; then echo "ERROR: cpp not found; install a host C preprocessor"; exit 1; fi; \
	if [ -z "$(AM62X_DTC)" ] || [ ! -x "$(AM62X_DTC)" ]; then echo "ERROR: dtc not found; add the AM62x toolchain bin directory to PATH"; exit 1; fi; \
	"$(AM62X_CPP)" \
		-I"$(OBC_TOP_DIR)/dts/am62x" \
		-I"$(KERNEL_SDK_DIR)/include" \
		-I"$(KERNEL_SDK_DIR)/include/uapi" \
		-I"$(KERNEL_SDK_DIR)/arch/arm64/boot/dts" \
		-I"$(KERNEL_SDK_DIR)/arch/arm64/boot/dts/ti" \
		-x assembler-with-cpp -P "$(AM62X_FDT_SOURCE)" -o "$(AM62X_FDT_PREPROCESSED)"; \
	"$(AM62X_DTC)" -I dts -O dtb -o "$(AM62X_FDT_BUILT)" "$(AM62X_FDT_PREPROCESSED)"; \
	echo "Built device tree sources: $(AM62X_FDT_PREPROCESSED) $(AM62X_FDT_BUILT)"

fdt_build_clean:
	rm -f "$(AM62X_FDT_TMP)" "$(AM62X_FDT_IMAGE)"

uboot_build_install: uboot_build
	@set -eu; \
	find_src() { for f in "$$1" "$$1_unsigned"; do if [ -f "$$f" ]; then echo "$$f"; return 0; fi; done; return 1; }; \
	uboot_src=$$(find_src "$(UBOOT_SDK_DIR)/u-boot.img") || { echo "ERROR: u-boot.img not found"; exit 1; }; \
	tispl_src=$$(find_src "$(UBOOT_SDK_DIR)/tispl.bin") || { echo "ERROR: tispl.bin not found"; exit 1; }; \
	if [ ! -f "$(AM62X_FDT_BUILT)" ]; then echo "ERROR: device tree output not found: $(AM62X_FDT_BUILT)"; exit 1; fi; \
	cp "$$uboot_src" "$(AM62X_UBOOT_TMP)"; cp "$$tispl_src" "$(AM62X_TISPL_TMP)"; cp "$(AM62X_FDT_BUILT)" "$(AM62X_FDT_TMP)"; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_UBOOT_TMP)" PACK_OUTPUT="$(AM62X_UBOOT_IMAGE)" PACK_HEAD_WRITE=1; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_TISPL_TMP)" PACK_OUTPUT="$(AM62X_TISPL_IMAGE)" PACK_HEAD_WRITE=1; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_FDT_TMP)" PACK_OUTPUT="$(AM62X_FDT_IMAGE)" PACK_HEAD_WRITE=1

uboot_build_clean: fdt_build_clean
	-$(MAKE) -C "$(UBOOT_SDK_DIR)" clean
	rm -f "$(AM62X_UBOOT_TMP)" "$(AM62X_TISPL_TMP)" "$(AM62X_UBOOT_IMAGE)" "$(AM62X_TISPL_IMAGE)"
uboot_clean: uboot_build_clean
