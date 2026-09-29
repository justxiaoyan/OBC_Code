IMX6ULL_KERNEL_CONFIG := $(OBC_TOP_DIR)/platform_config/imx6ull/sdk_config/kernel-imx6ull-defconfig
IMX6ULL_KERNEL_SOURCE := $(KERNEL_SDK_DIR)/arch/arm/boot/$(CONFIG_KERNEL_BIN_NAME)
IMX6ULL_FDT_SOURCE := $(KERNEL_SDK_DIR)/arch/arm/boot/dts/nxp/imx/$(CONFIG_FDT_BIN_NAME)
IMX6ULL_KERNEL_TMP := $(OBC_PACK_DIR)/$(CONFIG_KERNEL_BIN_NAME)
IMX6ULL_FDT_TMP := $(OBC_PACK_DIR)/$(CONFIG_FDT_BIN_NAME)
IMX6ULL_KERNEL_IMAGE := $(call OBC_UPGRADE_IMAGE,kernel)
IMX6ULL_FDT_IMAGE := $(call OBC_UPGRADE_IMAGE,fdt)

.PHONY: kernel kernel_build kernel_build_install kernel_build_clean kernel_clean fdt_build fdt_build_clean
kernel: kernel_build_install

kernel_build: check_sdk output
	@test -f "$(IMX6ULL_KERNEL_CONFIG)" || { echo "ERROR: kernel config not found: $(IMX6ULL_KERNEL_CONFIG)"; exit 1; }
	@test -n "$(OBC_CROSS_GCC)" || { echo "ERROR: cross compiler not found: $(OBC_TOOLCHAIN_PREFIX)gcc"; exit 1; }
	cp "$(IMX6ULL_KERNEL_CONFIG)" "$(KERNEL_SDK_DIR)/.config"
	$(MAKE) -C "$(KERNEL_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"
	$(MAKE) -C "$(KERNEL_SDK_DIR)" -j$$(nproc) zImage dtbs ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"
	@test -f "$(IMX6ULL_KERNEL_SOURCE)" || { echo "ERROR: kernel output not found: $(IMX6ULL_KERNEL_SOURCE)"; exit 1; }
	@test -f "$(IMX6ULL_FDT_SOURCE)" || { echo "ERROR: device tree output not found: $(IMX6ULL_FDT_SOURCE)"; exit 1; }

fdt_build: kernel_build

kernel_build_install: sign_tools kernel_build
	cp "$(IMX6ULL_KERNEL_SOURCE)" "$(IMX6ULL_KERNEL_TMP)"
	cp "$(IMX6ULL_FDT_SOURCE)" "$(IMX6ULL_FDT_TMP)"
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(IMX6ULL_KERNEL_TMP)" PACK_OUTPUT="$(IMX6ULL_KERNEL_IMAGE)" PACK_HEAD_WRITE=1
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(IMX6ULL_FDT_TMP)" PACK_OUTPUT="$(IMX6ULL_FDT_IMAGE)" PACK_HEAD_WRITE=1

fdt_build_clean:
	rm -f "$(IMX6ULL_FDT_TMP)" "$(IMX6ULL_FDT_IMAGE)"

kernel_build_clean: fdt_build_clean
	-$(MAKE) -C "$(KERNEL_SDK_DIR)" clean
	rm -f "$(IMX6ULL_KERNEL_TMP)" "$(IMX6ULL_KERNEL_IMAGE)"
kernel_clean: kernel_build_clean
