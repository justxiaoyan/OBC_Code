IMX6ULL_UBOOT_CONFIG := $(OBC_TOP_DIR)/platform_config/imx6ull/sdk_config/uboot-imx6ull-defconfig
IMX6ULL_UBOOT_SOURCE := $(UBOOT_SDK_DIR)/$(CONFIG_UBOOT_BIN_NAME)
IMX6ULL_UBOOT_TMP := $(OBC_PACK_DIR)/$(CONFIG_UBOOT_BIN_NAME)
IMX6ULL_UBOOT_IMAGE := $(call OBC_UPGRADE_IMAGE,uboot)

.PHONY: uboot uboot_build uboot_build_install uboot_build_clean uboot_clean
uboot: uboot_build_install

uboot_build: check_sdk output
	@test -f "$(IMX6ULL_UBOOT_CONFIG)" || { echo "ERROR: U-Boot config not found: $(IMX6ULL_UBOOT_CONFIG)"; exit 1; }
	@test -n "$(OBC_CROSS_GCC)" || { echo "ERROR: cross compiler not found: $(OBC_TOOLCHAIN_PREFIX)gcc"; exit 1; }
	cp "$(IMX6ULL_UBOOT_CONFIG)" "$(UBOOT_SDK_DIR)/.config"
	$(MAKE) -C "$(UBOOT_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"
	$(MAKE) -C "$(UBOOT_SDK_DIR)" -j$$(nproc) all ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"
	@test -f "$(IMX6ULL_UBOOT_SOURCE)" || { echo "ERROR: U-Boot output not found: $(IMX6ULL_UBOOT_SOURCE)"; exit 1; }

uboot_build_install: sign_tools uboot_build
	cp "$(IMX6ULL_UBOOT_SOURCE)" "$(IMX6ULL_UBOOT_TMP)"
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(IMX6ULL_UBOOT_TMP)" PACK_OUTPUT="$(IMX6ULL_UBOOT_IMAGE)" PACK_HEAD_WRITE=1

uboot_build_clean:
	-$(MAKE) -C "$(UBOOT_SDK_DIR)" clean
	rm -f "$(IMX6ULL_UBOOT_TMP)" "$(IMX6ULL_UBOOT_IMAGE)"
uboot_clean: uboot_build_clean
