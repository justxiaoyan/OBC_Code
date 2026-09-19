################################################################################
# OBC auto-tools recursive dispatcher
################################################################################

OBC_TOP_DIR ?= $(abspath $(CURDIR)/../..)
sinclude $(OBC_TOP_DIR)/.config

CROSS_COMPILE ?= $(subst ",,$(CONFIG_OBC_SDK_COMP))
TOOLCHAIN_SYSROOT ?= $(shell gcc=$(shell command -v $(CROSS_COMPILE)gcc 2>/dev/null); \
	root=$$(cd "$$(dirname "$$gcc")/.." 2>/dev/null && pwd); \
	triple=$(patsubst %-,%,$(CROSS_COMPILE)); \
	for candidate in "$$root/$$triple/sysroot" "$$root"/*/sysroot; do \
		if [ -d "$$candidate" ]; then echo "$$candidate"; break; fi; \
	done)

AUTO_TOOLS_OUTPUT_DIR := $(OBC_TOP_DIR)/output/auto-tools
AUTO_TOOLS_CHILD_DIRS := $(sort $(patsubst %/,%,$(wildcard */)))
AUTO_TOOLS_AUTOBUILD_DIRS := $(sort $(patsubst %/autobuild.sh,%,$(wildcard */autobuild.sh)))

auto_tools_config_key = CONFIG_AUTO_TOOLS_$(shell printf '%s' '$1' | sed -E 's/([a-z0-9])([A-Z])/\1_\2/g; s/[^[:alnum:]]/_/g' | tr '[:lower:]' '[:upper:]' | sed 's/^AUTO_//')
auto_tools_enabled = $(if $(filter y,$($(call auto_tools_config_key,$1))),y,$(shell if [ -f "$(OBC_TOP_DIR)/.config" ] && grep -q '^$(call auto_tools_config_key,$1)=y' "$(OBC_TOP_DIR)/.config"; then printf y; fi))
AUTO_TOOLS_AUTOBUILD_SUBDIRS := $(foreach dir,$(AUTO_TOOLS_AUTOBUILD_DIRS),$(if $(filter y,$(call auto_tools_enabled,$(dir))),$(dir)))

export OBC_TOP_DIR CROSS_COMPILE TOOLCHAIN_SYSROOT AUTO_TOOLS_OUTPUT_DIR

.PHONY: all build install clean show_config help \
	run-autobuild run-autobuild-install run-autobuild-clean

all: install

build: show_config run-autobuild

install: build run-autobuild-install
	@echo "Auto-tools installed in $(AUTO_TOOLS_OUTPUT_DIR)"

run-autobuild:
	@for dir in $(AUTO_TOOLS_AUTOBUILD_SUBDIRS); do \
		echo "Building auto-tools/$$dir..."; \
		( cd "$$dir" && CROSS_COMPILE="$(CROSS_COMPILE)" TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" sh ./autobuild.sh ) || exit 1; \
	done

run-autobuild-install:
	@mkdir -p "$(AUTO_TOOLS_OUTPUT_DIR)"; \
	for dir in $(AUTO_TOOLS_AUTOBUILD_SUBDIRS); do \
		echo "Installing auto-tools/$$dir..."; \
		( cd "$$dir" && DESTDIR="$(AUTO_TOOLS_OUTPUT_DIR)" CROSS_COMPILE="$(CROSS_COMPILE)" TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" sh ./autobuild.sh install ) || exit 1; \
	done

run-autobuild-clean:
	@for dir in $(AUTO_TOOLS_AUTOBUILD_DIRS); do \
		echo "Cleaning auto-tools/$$dir..."; \
		( cd "$$dir" && sh ./autobuild.sh clean ) || exit 1; \
	done

clean: show_config run-autobuild-clean
	@rm -rf "$(AUTO_TOOLS_OUTPUT_DIR)"

show_config:
	@echo "========================================"
	@echo "auto-tools Build Configuration"
	@echo "========================================"
	@echo "CROSS_COMPILE: $(CROSS_COMPILE)"
	@echo "OUTPUT_DIR:    $(AUTO_TOOLS_OUTPUT_DIR)"
	@echo "Child dirs:    $(if $(AUTO_TOOLS_CHILD_DIRS),$(AUTO_TOOLS_CHILD_DIRS),<none>)"
	@echo "Autobuild on:  $(if $(AUTO_TOOLS_AUTOBUILD_SUBDIRS),$(AUTO_TOOLS_AUTOBUILD_SUBDIRS),<none>)"
	@echo "========================================"

help:
	@echo "auto-tools recursively discovers direct child directories."
	@echo "Every auto-tools component must provide autobuild.sh."
