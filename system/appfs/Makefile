################################################################################
# appfs Makefile - 应用程序文件系统编译配置
################################################################################

# 获取顶层目录和配置. The top-level Makefile passes this explicitly;
# this fallback keeps direct builds from system/appfs working.
OBC_TOP_DIR ?= $(abspath $(CURDIR)/../..)
sinclude $(OBC_TOP_DIR)/.config

# 从 CONFIG_ 变量中提取实际值（去掉引号）
CROSS_COMPILE := $(subst ",,$(CONFIG_OBC_SDK_COMP))
ARCH := $(subst ",,$(CONFIG_OBC_SDK_ARCH))
TOOLCHAIN_SYSROOT ?= $(shell gcc=$(shell command -v $(CROSS_COMPILE)gcc 2>/dev/null); \
	root=$$(cd "$$(dirname "$$gcc")/.." 2>/dev/null && pwd); \
	triple=$(patsubst %-,%,$(CROSS_COMPILE)); \
	for candidate in "$$root/$$triple/sysroot" "$$root"/*/sysroot; do \
		if [ -d "$$candidate" ]; then echo "$$candidate"; break; fi; \
	done)
APPFS_CFLAGS ?= $(if $(TOOLCHAIN_SYSROOT),--sysroot=$(TOOLCHAIN_SYSROOT),)
export OBC_TOP_DIR TOOLCHAIN_SYSROOT APPFS_CFLAGS

# 输出目录
APPFS_OUTPUT_DIR := $(OBC_TOP_DIR)/output/appfs
export APPFS_OUTPUT_DIR

################################################################################
# Component discovery
################################################################################

# Discover only direct child directories. Build output nested below a child
# cannot accidentally become a component.
APPFS_CHILD_DIRS := $(sort $(patsubst %/,%,$(wildcard */)))
APPFS_AUTOBUILD_DIRS := $(sort $(patsubst %/autobuild.sh,%,$(wildcard */autobuild.sh)))

# Convert a directory name such as amp-a53 to CONFIG_APPFS_AMP_A53. An
# Explicit CONFIG_APPFS_<name>=y enables a component and n disables it. The
# directory-to-symbol conversion keeps this Makefile independent of component
# names; adding a new autobuild.sh never requires a parent Makefile edit.
appfs_config_key = CONFIG_APPFS_$(shell printf '%s' '$1' | sed -E 's/([a-z0-9])([A-Z])/\1_\2/g; s/[^[:alnum:]]/_/g' | tr '[:lower:]' '[:upper:]')
appfs_config_state = $(if $(filter y,$($(call appfs_config_key,$1))),y,$(if $(filter n,$($(call appfs_config_key,$1))),n,$(shell if [ -f "$(OBC_TOP_DIR)/.config" ]; then if grep -q '^$(call appfs_config_key,$1)=y' "$(OBC_TOP_DIR)/.config"; then printf y; elif grep -q '^# $(call appfs_config_key,$1) is not set' "$(OBC_TOP_DIR)/.config"; then printf n; fi; fi)))
APPFS_AUTOBUILD_SUBDIRS := $(foreach dir,$(APPFS_AUTOBUILD_DIRS),$(if $(filter y,$(call appfs_config_state,$(dir))),$(dir)))

################################################################################
# Build and install dispatch
################################################################################

.PHONY: all clean install help show_config \
	run-autobuild run-autobuild-install run-autobuild-clean

all: show_config run-autobuild

install: show_config run-autobuild-install
	@echo ""
	@echo "Installation complete. Installed files:"
	@mkdir -p "$(APPFS_OUTPUT_DIR)"
	@find "$(APPFS_OUTPUT_DIR)" -maxdepth 1 -type f -printf '  %f (%s bytes)\n' | sort
	@echo ""
	@echo "Installation directory: $(APPFS_OUTPUT_DIR)"

run-autobuild:
	@if [ -z "$(APPFS_AUTOBUILD_SUBDIRS)" ]; then \
		echo "No enabled appfs autobuild components found."; \
	else \
		mkdir -p "$(APPFS_OUTPUT_DIR)"; \
		dirs="$(APPFS_AUTOBUILD_SUBDIRS)"; \
		for dir in $$dirs; do \
			echo ""; \
			echo "========================================"; \
			echo "Autobuild $$dir..."; \
			echo "========================================"; \
			( cd "$$dir" && \
			  OBC_TOP_DIR="$(OBC_TOP_DIR)" \
			  APPFS_OUTPUT_DIR="$(APPFS_OUTPUT_DIR)" \
			  DESTDIR="$(APPFS_OUTPUT_DIR)" \
			  CROSS_COMPILE="$(CROSS_COMPILE)" \
			  TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" \
			  sh ./autobuild.sh ) || exit 1; \
		done; \
	fi

run-autobuild-install:
	@if [ -z "$(APPFS_AUTOBUILD_SUBDIRS)" ]; then \
		echo "No enabled appfs autobuild components to install."; \
	else \
		mkdir -p "$(APPFS_OUTPUT_DIR)"; \
		dirs="$(APPFS_AUTOBUILD_SUBDIRS)"; \
		for dir in $$dirs; do \
			echo ""; \
			echo "========================================"; \
			echo "Autobuild and install $$dir..."; \
			echo "========================================"; \
			( cd "$$dir" && \
			  OBC_TOP_DIR="$(OBC_TOP_DIR)" \
			  APPFS_OUTPUT_DIR="$(APPFS_OUTPUT_DIR)" \
			  DESTDIR="$(APPFS_OUTPUT_DIR)" \
			  CROSS_COMPILE="$(CROSS_COMPILE)" \
			  TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" \
			  sh ./autobuild.sh install ) || exit 1; \
		done; \
	fi

run-autobuild-clean:
	@if [ -z "$(APPFS_AUTOBUILD_SUBDIRS)" ]; then \
		echo "No enabled appfs autobuild components to clean."; \
	else \
		dirs="$(APPFS_AUTOBUILD_SUBDIRS)"; \
		for dir in $$dirs; do \
			echo "Cleaning autobuild $$dir..."; \
			( cd "$$dir" && sh ./autobuild.sh clean ) || exit 1; \
		done; \
	fi

clean: show_config run-autobuild-clean
	@rm -rf "$(APPFS_OUTPUT_DIR)"
	@echo "Clean complete."

show_config:
	@echo "========================================"
	@echo "appfs Build Configuration"
	@echo "========================================"
	@echo "CROSS_COMPILE: $(CROSS_COMPILE)"
	@echo "ARCH:          $(ARCH)"
	@echo "OUTPUT_DIR:    $(APPFS_OUTPUT_DIR)"
	@echo "Child dirs:     $(if $(APPFS_CHILD_DIRS),$(APPFS_CHILD_DIRS),<none>)"
	@echo "Autobuild dirs: $(if $(APPFS_AUTOBUILD_DIRS),$(APPFS_AUTOBUILD_DIRS),<none>)"
	@echo "Autobuild on:   $(if $(APPFS_AUTOBUILD_SUBDIRS),$(APPFS_AUTOBUILD_SUBDIRS),<none>)"
	@echo "========================================"

help:
	@echo "appfs Makefile - Application File System Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all enabled applications (default)"
	@echo "  install      - Install built applications to output directory"
	@echo "  clean        - Clean enabled applications and installed outputs"
	@echo "  show_config  - Display current build configuration"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  Directories containing autobuild.sh are discovered automatically."
	@echo "  autobuild.sh must support the default build and install/clean arguments."
	@echo "  CONFIG_APPFS_<directory> = y enables an autobuild component."
	@echo "  Every appfs component must provide autobuild.sh."
	@echo "  Use 'make menuconfig' in the top directory to configure."
	@echo ""
	@echo "Currently enabled applications:"
	@if [ -z "$(APPFS_AUTOBUILD_SUBDIRS)" ]; then \
		echo "  <none>"; \
	else \
		dirs="$(APPFS_AUTOBUILD_SUBDIRS)"; \
		for dir in $$dirs; do \
			echo "  - $$dir"; \
		done; \
	fi
