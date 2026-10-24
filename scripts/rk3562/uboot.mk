RK3562_UBOOT_CONFIG := $(OBC_TOP_DIR)/platform_config/rk3562/sdk_config/uboot-rk3562-defconfig
RK3562_UBOOT_SOURCE := $(UBOOT_SDK_DIR)/$(CONFIG_UBOOT_BIN_NAME)
RK3562_UBOOT_TMP := $(OBC_PACK_DIR)/$(CONFIG_UBOOT_BIN_NAME)
RK3562_UBOOT_IMAGE := $(call OBC_UPGRADE_IMAGE,uboot)
RK3562_LOADER_SOURCE := $(UBOOT_SDK_DIR)/$(CONFIG_LOADER_BIN_NAME)
RK3562_LOADER_TMP := $(OBC_PACK_DIR)/$(CONFIG_LOADER_BIN_NAME)
RK3562_LOADER_IMAGE := $(call OBC_UPGRADE_IMAGE,loader)
RK3562_SPL_SOURCE := $(UBOOT_SDK_DIR)/$(subst ",,$(CONFIG_SPL_BIN_NAME))
RK3562_RKBIN_DIR := $(UBOOT_SDK_DIR)/$(subst ",,$(CONFIG_RKBIN_PATH))
RK3562_UBOOT_PATH := $(UBOOT_SDK_DIR)/scripts/dtc:/usr/bin:/bin:$(OBC_TOOLCHAIN_ROOT)/bin

.PHONY: loader loader_build loader_build_install loader_clean uboot uboot_build uboot_build_install uboot_clean
uboot: uboot_build_install
loader: loader_build_install

uboot_build: check_sdk output
	@test -f "$(RK3562_UBOOT_CONFIG)" || { echo "ERROR: U-Boot config not found: $(RK3562_UBOOT_CONFIG)"; exit 1; }
	@test -n "$(OBC_CROSS_GCC)" || { echo "ERROR: cross compiler not found: $(OBC_TOOLCHAIN_PREFIX)gcc"; exit 1; }
	@test -d "$(RK3562_RKBIN_DIR)/RKBOOT" && test -d "$(RK3562_RKBIN_DIR)/RKTRUST" || { echo "ERROR: rkbin not found: $(RK3562_RKBIN_DIR)"; exit 1; }
	@command -v fdtget >/dev/null 2>&1 || { echo "ERROR: fdtget not found; install device-tree-compiler for Rockchip FIT packaging"; exit 127; }
	cp "$(RK3562_UBOOT_CONFIG)" "$(UBOOT_SDK_DIR)/.config"
	$(MAKE) -C "$(UBOOT_SDK_DIR)" olddefconfig ARCH=arm CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)"
	cd "$(UBOOT_SDK_DIR)" && PATH="$(RK3562_UBOOT_PATH):$$PATH" ./make.sh "CROSS_COMPILE=$(OBC_TOOLCHAIN_PREFIX)"
	@test -s "$(RK3562_UBOOT_SOURCE)" || { echo "ERROR: Rockchip U-Boot image not found: $(RK3562_UBOOT_SOURCE)"; exit 1; }
	@test -s "$(RK3562_SPL_SOURCE)" || { echo "ERROR: built SPL not found: $(RK3562_SPL_SOURCE)"; exit 1; }
	cd "$(UBOOT_SDK_DIR)" && ./make.sh --spl "CROSS_COMPILE=$(OBC_TOOLCHAIN_PREFIX)"
	@test -s "$(RK3562_LOADER_SOURCE)" || { echo "ERROR: Rockchip loader not found: $(RK3562_LOADER_SOURCE)"; exit 1; }

uboot_build_install: sign_tools uboot_build
	cp "$(RK3562_UBOOT_SOURCE)" "$(RK3562_UBOOT_TMP)"
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(RK3562_UBOOT_TMP)" PACK_OUTPUT="$(RK3562_UBOOT_IMAGE)" PACK_HEAD_WRITE=1

loader_build: uboot_build

loader_build_install: sign_tools loader_build
	cp "$(RK3562_LOADER_SOURCE)" "$(RK3562_LOADER_TMP)"
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(RK3562_LOADER_TMP)" PACK_OUTPUT="$(RK3562_LOADER_IMAGE)" PACK_HEAD_WRITE=0

loader_clean:
	rm -f "$(RK3562_LOADER_TMP)" "$(RK3562_LOADER_IMAGE)"

uboot_clean: loader_clean
	-$(MAKE) -C "$(UBOOT_SDK_DIR)" clean
	rm -f "$(RK3562_UBOOT_TMP)" "$(RK3562_UBOOT_IMAGE)"
