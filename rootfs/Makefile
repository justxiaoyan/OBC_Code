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
ROOTFS_OUTPUT_FILE ?= $(OBC_TOP_DIR)/scripts/am62x/rootfs.cpio.gz

BUSYBOX_CFLAGS := $(if $(SYSROOT),--sysroot=$(SYSROOT),)
BUSYBOX_LDFLAGS := $(if $(SYSROOT),--sysroot=$(SYSROOT),)

.PHONY: all busybox busybox_install rootfs_pack clean busybox_clean menuconfig

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

rootfs_pack: busybox
	@mkdir -p "$(OBC_TOP_DIR)/output/tmp"
	@export CROSS_COMPILE="$(CROSS_COMPILE)" SYSROOT="$(SYSROOT)" \
		ROOTFS_STAGING_DIR="$(ROOTFS_STAGING_DIR)" ROOTFS_OUTPUT_FILE="$(ROOTFS_OUTPUT_FILE)"; \
		if command -v fakeroot >/dev/null 2>&1; then \
			fakeroot sh "$(ROOTFS_DIR)/pack_rootfs.sh"; \
		else \
			sh "$(ROOTFS_DIR)/pack_rootfs.sh"; \
		fi

busybox_clean:
	rm -rf "$(BUSYBOX_BUILD_DIR)"

clean:
	rm -rf "$(BUSYBOX_BUILD_DIR)" "$(ROOTFS_STAGING_DIR)"
	rm -f "$(ROOTFS_OUTPUT_FILE).tmp"

menuconfig:
	kconfig-mconf "$(ROOTFS_DIR)/Kconfig"
