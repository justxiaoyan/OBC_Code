ROOTFS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OBC_TOP_DIR := $(abspath $(ROOTFS_DIR)/..)

ROOTFS_BUSYBOX_DIR ?= $(ROOTFS_DIR)/busybox
ROOTFS_BUSYBOX_VER ?= 1.36.1
BUSYBOX_SOURCE_DIR := $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER)
BUSYBOX_ARCHIVE := $(ROOTFS_BUSYBOX_DIR)/busybox-$(ROOTFS_BUSYBOX_VER).tar.bz2
BUSYBOX_BUILD_DIR ?= $(OBC_TOP_DIR)/output/tmp/busybox-build
BUSYBOX_CONFIG := $(ROOTFS_BUSYBOX_DIR)/busybox1.36.1_defconfig

ARCH ?= arm64
CROSS_COMPILE ?= aarch64-ca53-linux-gnu-
SYSROOT ?=
ROOTFS_STAGING_DIR ?= $(OBC_TOP_DIR)/output/tmp/install
ROOTFS_OUTPUT_FILE ?= $(OBC_TOP_DIR)/output/tmp/rootfs.sparse.bin
MAKE_EXT4FS ?= make_ext4fs
FAKEROOT ?= $(if $(wildcard /usr/bin/fakeroot),/usr/bin/fakeroot,fakeroot)
KERNEL_SDK_DIR ?= $(OBC_TOP_DIR)/../am62x_sdk_source/kernel
KERNEL_CONFIG ?= $(OBC_TOP_DIR)/platform_config/am62x/sdk_config/kernel-am62x-defconfig
OBCBASE_SOURCE_DIR := $(OBC_TOP_DIR)/module/obcbase
OBCBASE_BUILD_DIR ?= $(OBC_TOP_DIR)/output/tmp/obcbase-build
OBCBASE_MODULE := $(OBCBASE_BUILD_DIR)/obcbase.ko
OBCTOOL_SOURCE_DIR := $(OBC_TOP_DIR)/system/appfs/obctool
OBCTOOL_BUILD_DIR ?= $(OBC_TOP_DIR)/output/tmp/obctool-build
CONFIG_OBC_BASE ?= y

BUSYBOX_CFLAGS := $(if $(SYSROOT),--sysroot=$(SYSROOT),)
BUSYBOX_LDFLAGS := $(if $(SYSROOT),--sysroot=$(SYSROOT),)

.PHONY: all busybox busybox_install kernel_modules_prepare obcbase \
	obcbase_install obctool obctool_install rootfs_pack clean \
	busybox_clean obcbase_clean obctool_clean menuconfig

all: rootfs_pack

busybox:
	@test -f "$(BUSYBOX_CONFIG)" || { echo "ERROR: BusyBox config not found: $(BUSYBOX_CONFIG)"; exit 1; }
	@test -f "$(BUSYBOX_ARCHIVE)" || { echo "ERROR: BusyBox archive not found: $(BUSYBOX_ARCHIVE)"; exit 1; }
	@if [ ! -f "$(BUSYBOX_SOURCE_DIR)/scripts/Makefile.lib" ]; then \
		tar -xjf "$(BUSYBOX_ARCHIVE)" -C "$(ROOTFS_BUSYBOX_DIR)"; \
	fi
	@rm -rf "$(BUSYBOX_BUILD_DIR)"
	@mkdir -p "$(BUSYBOX_BUILD_DIR)"
	$(MAKE) -C "$(BUSYBOX_SOURCE_DIR)" O="$(BUSYBOX_BUILD_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" allnoconfig
	@while IFS='=' read -r symbol value; do \
		case "$$symbol" in ''|\#*) continue ;; esac; \
		if grep -q "^# $${symbol} is not set$$" "$(BUSYBOX_BUILD_DIR)/.config"; then \
			sed -i "s|^# $${symbol} is not set$$|$${symbol}=$${value}|" "$(BUSYBOX_BUILD_DIR)/.config"; \
		elif grep -q "^$${symbol}=" "$(BUSYBOX_BUILD_DIR)/.config"; then \
			sed -i "s|^$${symbol}=.*$$|$${symbol}=$${value}|" "$(BUSYBOX_BUILD_DIR)/.config"; \
		else \
			printf '%s=%s\\n' "$$symbol" "$$value" >> "$(BUSYBOX_BUILD_DIR)/.config"; \
		fi; \
	done < "$(BUSYBOX_CONFIG)"
	@yes '' | $(MAKE) -C "$(BUSYBOX_SOURCE_DIR)" O="$(BUSYBOX_BUILD_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" oldconfig
	$(MAKE) -C "$(BUSYBOX_SOURCE_DIR)" O="$(BUSYBOX_BUILD_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" \
		CFLAGS="$(BUSYBOX_CFLAGS)" LDFLAGS="$(BUSYBOX_LDFLAGS)"

busybox_install: busybox
	@mkdir -p "$(ROOTFS_STAGING_DIR)"
	$(MAKE) -C "$(BUSYBOX_SOURCE_DIR)" O="$(BUSYBOX_BUILD_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" \
		CFLAGS="$(BUSYBOX_CFLAGS)" LDFLAGS="$(BUSYBOX_LDFLAGS)" \
		CONFIG_PREFIX="$(ROOTFS_STAGING_DIR)" install

kernel_modules_prepare:
	@test -d "$(KERNEL_SDK_DIR)" || { echo "ERROR: kernel source not found: $(KERNEL_SDK_DIR)"; exit 1; }
	@test -f "$(KERNEL_CONFIG)" || { echo "ERROR: kernel config not found: $(KERNEL_CONFIG)"; exit 1; }
	cp "$(KERNEL_CONFIG)" "$(KERNEL_SDK_DIR)/.config"
	$(MAKE) -C "$(KERNEL_SDK_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" olddefconfig
	$(MAKE) -C "$(KERNEL_SDK_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" modules_prepare

obcbase: kernel_modules_prepare
	@test "$(CONFIG_OBC_BASE)" = y || { echo "ERROR: CONFIG_OBC_BASE must be enabled for this rootfs"; exit 1; }
	$(MAKE) -C "$(OBCBASE_SOURCE_DIR)" all \
		KDIR="$(KERNEL_SDK_DIR)" ARCH="$(ARCH)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" BUILD_DIR="$(OBCBASE_BUILD_DIR)"

obcbase_install: obcbase
	@mkdir -p "$(ROOTFS_STAGING_DIR)/usr/obc/ko"
	install -m 0644 "$(OBCBASE_MODULE)" "$(ROOTFS_STAGING_DIR)/usr/obc/ko/obcbase.ko"

obctool:
	$(MAKE) -C "$(OBCTOOL_SOURCE_DIR)" all \
		OBC_TOP_DIR="$(OBC_TOP_DIR)" BUILD_DIR="$(OBCTOOL_BUILD_DIR)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		CFLAGS="-Os $(BUSYBOX_CFLAGS) -Wall -Wextra -Werror" \
		LDFLAGS="$(BUSYBOX_LDFLAGS)"

obctool_install: obctool
	$(MAKE) -C "$(OBCTOOL_SOURCE_DIR)" install \
		OBC_TOP_DIR="$(OBC_TOP_DIR)" BUILD_DIR="$(OBCTOOL_BUILD_DIR)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" DESTDIR="$(ROOTFS_STAGING_DIR)"

rootfs_pack:
	@mkdir -p "$(OBC_TOP_DIR)/output/tmp"
	@export CROSS_COMPILE="$(CROSS_COMPILE)" SYSROOT="$(SYSROOT)" \
		ARCH="$(ARCH)" ROOTFS_STAGING_DIR="$(ROOTFS_STAGING_DIR)" \
		ROOTFS_OUTPUT_FILE="$(ROOTFS_OUTPUT_FILE)" \
		MAKE_EXT4FS="$(MAKE_EXT4FS)" \
		KERNEL_SDK_DIR="$(KERNEL_SDK_DIR)" KERNEL_CONFIG="$(KERNEL_CONFIG)" \
		OBCBASE_BUILD_DIR="$(OBCBASE_BUILD_DIR)" \
		OBCTOOL_BUILD_DIR="$(OBCTOOL_BUILD_DIR)" \
		CONFIG_OBC_BASE="$(CONFIG_OBC_BASE)"; \
		if command -v "$(FAKEROOT)" >/dev/null 2>&1; then \
			if ! "$(FAKEROOT)" sh -c 'test "$$(id -u)" -eq 0'; then \
				echo "ERROR: host fakeroot cannot start: $$(command -v "$(FAKEROOT)")" >&2; \
				echo "       Check the fakeroot executable and its library, or set FAKEROOT=/usr/bin/fakeroot-sysv" >&2; \
				exit 1; \
			fi; \
			"$(FAKEROOT)" sh "$(ROOTFS_DIR)/pack_rootfs.sh"; \
		else \
			sh "$(ROOTFS_DIR)/pack_rootfs.sh"; \
		fi

busybox_clean:
	rm -rf "$(BUSYBOX_BUILD_DIR)"

obcbase_clean:
	rm -rf "$(OBCBASE_BUILD_DIR)"

obctool_clean:
	rm -rf "$(OBCTOOL_BUILD_DIR)"

clean: busybox_clean obcbase_clean obctool_clean
	rm -rf "$(ROOTFS_STAGING_DIR)"
	rm -f "$(ROOTFS_OUTPUT_FILE).tmp"

menuconfig:
	kconfig-mconf "$(ROOTFS_DIR)/Kconfig"
