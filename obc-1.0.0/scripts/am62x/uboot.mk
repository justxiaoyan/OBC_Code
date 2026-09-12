AM62X_UBOOT_TMP := $(OBC_PACK_DIR)/$(subst ",,$(CONFIG_UBOOT_BIN_NAME))
AM62X_TISPL_TMP := $(OBC_PACK_DIR)/$(subst ",,$(CONFIG_TISPL_BIN_NAME))
AM62X_FDT_SOURCE := $(OBC_TOP_DIR)/dts/am62x/k3-am625-sk.dts
AM62X_FDT_TMP := $(OBC_PACK_DIR)/$(subst ",,$(CONFIG_FDT_BIN_NAME))
AM62X_FDT_IMAGE := $(call OBC_UPGRADE_IMAGE,fdt)
AM62X_FDT_BUILD_DIR := $(OBC_TOP_DIR)/dts/am62x
AM62X_FDT_BUILT := $(AM62X_FDT_BUILD_DIR)/k3-am625-sk.dtb
AM62X_FDT_PREPROCESSED := $(AM62X_FDT_BUILD_DIR)/k3-am625-sk.temp.dts
AM62X_CPP := $(shell command -v cpp 2>/dev/null)
AM62X_DTC := $(shell command -v dtc 2>/dev/null)
AM62X_UBOOT_IMAGE := $(call OBC_UPGRADE_IMAGE,uboot)
AM62X_TISPL_IMAGE := $(call OBC_UPGRADE_IMAGE,teeos)
AM62X_LEGACY_FDT_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-fdt.bin
AM62X_LEGACY_UBOOT_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-uboot.bin
AM62X_LEGACY_TISPL_IMAGE := $(OBC_PACK_IMAGE_DIR)/100p-teeos.bin
AM62X_UBOOT_CONFIG := $(OBC_TOP_DIR)/platform_config/am62x/sdk_config/uboot-am62x-defconfig

# The configured tiboot3 variant determines which main-domain images are
# bootable on the target. TI K3 GP devices must use the unsigned FIT images;
# signed FIT payloads start with an X.509 certificate and cannot be executed
# when the A53 SPL does not enable FIT image post-processing.
ifneq ($(findstring -gp-,$(CONFIG_R5_LOADER_BIN_NAME)),)
AM62X_DEVICE_TYPE := GP
AM62X_UBOOT_SOURCE := $(UBOOT_SDK_DIR)/$(subst ",,$(CONFIG_UBOOT_BIN_NAME))_unsigned
AM62X_TISPL_SOURCE := $(UBOOT_SDK_DIR)/$(subst ",,$(CONFIG_TISPL_BIN_NAME))_unsigned
else
AM62X_DEVICE_TYPE := HS
AM62X_UBOOT_SOURCE := $(UBOOT_SDK_DIR)/$(subst ",,$(CONFIG_UBOOT_BIN_NAME))
AM62X_TISPL_SOURCE := $(UBOOT_SDK_DIR)/$(subst ",,$(CONFIG_TISPL_BIN_NAME))
endif

OBCBASE_SOURCE := $(OBC_TOP_DIR)/bootloader/obcbase
OBCBASE_SDK := $(UBOOT_SDK_DIR)/obcbase
AM62X_UBOOT_DTS_OVERLAY_REL := arch/arm/dts/k3-am625-alientek-u-boot.dtsi
AM62X_UBOOT_DTS_OVERLAY_SOURCE := $(OBCBASE_SOURCE)/board/am62x/k3-am625-alientek-u-boot.dtsi
AM62X_UBOOT_DTS_OVERLAY_TARGET := $(UBOOT_SDK_DIR)/$(AM62X_UBOOT_DTS_OVERLAY_REL)
OBCBASE_PATCH_FILES := Kconfig Makefile common/board_r.c common/spl/spl.c common/spl/spl_mmc.c include/spl.h arch/arm/mach-k3/am62x/am625_init.c arch/arm/mach-k3/include/mach/am62_hardware.h dts/upstream/src/arm64/ti/k3-am625-sk.dts $(AM62X_UBOOT_DTS_OVERLAY_REL)

.PHONY: uboot uboot_build uboot_build_install uboot_build_clean fdt_build fdt_build_clean obcbase_sync obcbase_clean
uboot: uboot_build_install

uboot_build: check_sdk output fdt_build
	@set -eu; \
	cleanup() { \
		git -C "$(UBOOT_SDK_DIR)" restore -- $(OBCBASE_PATCH_FILES); \
		find "$(UBOOT_SDK_DIR)/obcbase" -depth -type f -delete 2>/dev/null || true; \
		find "$(UBOOT_SDK_DIR)/obcbase" -depth -type d -empty -delete 2>/dev/null || true; \
	}; \
	trap cleanup EXIT INT TERM; \
	$(MAKE) obcbase_sync; \
	if [ ! -f "$(AM62X_UBOOT_CONFIG)" ]; then echo "ERROR: U-Boot defconfig not found: $(AM62X_UBOOT_CONFIG)"; exit 1; fi; \
	cp "$(AM62X_UBOOT_CONFIG)" "$(UBOOT_SDK_DIR)/.config"; \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" $(UBOOT_TOOLCHAIN_VARS); \
	$(MAKE) -C "$(UBOOT_SDK_DIR)" -j$$(nproc) all ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" $(UBOOT_TOOLCHAIN_VARS)

obcbase_sync: check_sdk
	@set -eu; \
	if [ ! -d "$(OBCBASE_SOURCE)" ]; then echo "ERROR: OBC base source not found: $(OBCBASE_SOURCE)"; exit 1; fi; \
	if [ ! -f "$(AM62X_UBOOT_DTS_OVERLAY_SOURCE)" ]; then echo "ERROR: U-Boot DTS overlay not found: $(AM62X_UBOOT_DTS_OVERLAY_SOURCE)"; exit 1; fi; \
	mkdir -p "$(OBCBASE_SDK)"; \
	find "$(OBCBASE_SDK)" -type f -delete; \
	cp -a "$(OBCBASE_SOURCE)/." "$(OBCBASE_SDK)/"; \
	cp "$(AM62X_UBOOT_DTS_OVERLAY_SOURCE)" "$(AM62X_UBOOT_DTS_OVERLAY_TARGET)"; \
	for f in $(OBCBASE_PATCH_FILES); do \
		sed -i -e 's#emsbase#obcbase#g' -e 's/CONFIG_EMS_/CONFIG_OBC_/g' -e 's/EMS_PACK/OBC_PACK/g' -e 's/EMS_MAGIC/OBC_MAGIC/g' -e 's/EMS_HEADER/OBC_HEADER/g' -e 's/EMSFS/OBCFS/g' -e 's/emspart/obcpart/g' -e 's/ems_/obc_/g' -e 's/ems-/obc-/g' -e 's/do_emsboot/do_obcboot/g' "$(UBOOT_SDK_DIR)/$$f"; \
	done

obcbase_clean:
	@set -eu; \
	git -C "$(UBOOT_SDK_DIR)" restore -- $(OBCBASE_PATCH_FILES); \
	find "$(OBCBASE_SDK)" -depth -type f -delete 2>/dev/null || true; \
	find "$(OBCBASE_SDK)" -depth -type d -empty -delete 2>/dev/null || true

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
	rm -f "$(AM62X_FDT_TMP)" "$(AM62X_FDT_IMAGE)" "$(AM62X_LEGACY_FDT_IMAGE)"

uboot_build_install: sign_tools uboot_build
	@set -eu; \
	uboot_src="$(AM62X_UBOOT_SOURCE)"; \
	tispl_src="$(AM62X_TISPL_SOURCE)"; \
	if [ ! -f "$$uboot_src" ]; then echo "ERROR: $(AM62X_DEVICE_TYPE) U-Boot image not found: $$uboot_src"; exit 1; fi; \
	if [ ! -f "$$tispl_src" ]; then echo "ERROR: $(AM62X_DEVICE_TYPE) TI SPL image not found: $$tispl_src"; exit 1; fi; \
	if [ ! -f "$(AM62X_FDT_BUILT)" ]; then echo "ERROR: device tree output not found: $(AM62X_FDT_BUILT)"; exit 1; fi; \
	echo "Using $(AM62X_DEVICE_TYPE) boot images: $$(basename "$$tispl_src") $$(basename "$$uboot_src")"; \
	rm -f "$(AM62X_LEGACY_UBOOT_IMAGE)" "$(AM62X_LEGACY_TISPL_IMAGE)" "$(AM62X_LEGACY_FDT_IMAGE)"; \
	cp "$$uboot_src" "$(AM62X_UBOOT_TMP)"; cp "$$tispl_src" "$(AM62X_TISPL_TMP)"; cp "$(AM62X_FDT_BUILT)" "$(AM62X_FDT_TMP)"; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_UBOOT_TMP)" PACK_OUTPUT="$(AM62X_UBOOT_IMAGE)" PACK_HEAD_WRITE=1; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_TISPL_TMP)" PACK_OUTPUT="$(AM62X_TISPL_IMAGE)" PACK_HEAD_WRITE=1; \
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(AM62X_FDT_TMP)" PACK_OUTPUT="$(AM62X_FDT_IMAGE)" PACK_HEAD_WRITE=1

uboot_build_clean: fdt_build_clean obcbase_clean
	-$(MAKE) -C "$(UBOOT_SDK_DIR)" clean
	rm -f "$(AM62X_UBOOT_TMP)" "$(AM62X_TISPL_TMP)" \
		"$(AM62X_UBOOT_IMAGE)" "$(AM62X_TISPL_IMAGE)" \
		"$(AM62X_LEGACY_UBOOT_IMAGE)" "$(AM62X_LEGACY_TISPL_IMAGE)"
uboot_clean: uboot_build_clean
