RK3562_KERNEL_CONFIG := $(OBC_TOP_DIR)/platform_config/rk3562/sdk_config/kernel-rk3562-defconfig
RK3562_KERNEL_SOURCE := $(KERNEL_SDK_DIR)/arch/arm64/boot/$(CONFIG_KERNEL_BIN_NAME)
RK3562_KERNEL_TMP := $(OBC_PACK_DIR)/$(CONFIG_KERNEL_BIN_NAME)
RK3562_KERNEL_IMAGE := $(call OBC_UPGRADE_IMAGE,kernel)
# The SDK toolchain contains a relocated host Perl whose @INC points at the
# original SDK checkout. Keep host utilities ahead of it while retaining the
# cross compiler in the toolchain bin directory.
RK3562_KERNEL_PATH := /usr/bin:/bin:$(OBC_TOOLCHAIN_ROOT)/bin

.PHONY: kernel kernel_build kernel_build_install kernel_clean fdt_build fdt_build_clean
kernel: kernel_build_install

kernel_build: check_sdk output
	@test -f "$(RK3562_KERNEL_CONFIG)" || { echo "ERROR: kernel config not found: $(RK3562_KERNEL_CONFIG)"; exit 1; }
	@test -n "$(OBC_CROSS_GCC)" || { echo "ERROR: cross compiler not found: $(OBC_TOOLCHAIN_PREFIX)gcc"; exit 1; }
	cp "$(RK3562_KERNEL_CONFIG)" "$(KERNEL_SDK_DIR)/.config"
	PATH="$(RK3562_KERNEL_PATH):$$PATH" LD_LIBRARY_PATH="$(OBC_TOOLCHAIN_ROOT)/lib:$${LD_LIBRARY_PATH:-}" $(MAKE) -C "$(KERNEL_SDK_DIR)" olddefconfig ARCH=arm64 CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" CC="$(OBC_TOOLCHAIN_PREFIX)gcc"
	PATH="$(RK3562_KERNEL_PATH):$$PATH" LD_LIBRARY_PATH="$(OBC_TOOLCHAIN_ROOT)/lib:$${LD_LIBRARY_PATH:-}" $(MAKE) -C "$(KERNEL_SDK_DIR)" -j$$(nproc) "$(subst ",,$(CONFIG_KERNEL_BUILD_TARGET))" ARCH=arm64 CROSS_COMPILE="$(OBC_TOOLCHAIN_PREFIX)" CC="$(OBC_TOOLCHAIN_PREFIX)gcc"
	@test -s "$(RK3562_KERNEL_SOURCE)" || { echo "ERROR: kernel image not found: $(RK3562_KERNEL_SOURCE)"; exit 1; }

fdt_build:
	@echo "ERROR: separate RK3562 device tree build is not enabled yet"; exit 1

kernel_build_install: sign_tools kernel_build
	cp "$(RK3562_KERNEL_SOURCE)" "$(RK3562_KERNEL_TMP)"
	$(MAKE) -C "$(OBC_TOP_DIR)" pack-signed PACK_INPUT="$(RK3562_KERNEL_TMP)" PACK_OUTPUT="$(RK3562_KERNEL_IMAGE)" PACK_HEAD_WRITE=1

fdt_build_clean:
	rm -f "$(OBC_PACK_DIR)/$(CONFIG_FDT_BIN_NAME)" "$(call OBC_UPGRADE_IMAGE,fdt)"

kernel_clean: fdt_build_clean
	-$(MAKE) -C "$(KERNEL_SDK_DIR)" clean
	rm -f "$(RK3562_KERNEL_TMP)" "$(RK3562_KERNEL_IMAGE)"
