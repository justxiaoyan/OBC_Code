################################################################################
# OBC auto-tools build configuration
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
AUTO_TOOLS_SUBDIRS-$(CONFIG_AUTO_TOOLS_MEM)      += auto-mem
AUTO_TOOLS_SUBDIRS-$(CONFIG_AUTO_TOOLS_SMART)    += auto-smart
AUTO_TOOLS_SUBDIRS-$(CONFIG_AUTO_TOOLS_PRIORITY) += auto-priority
AUTO_TOOLS_SUBDIRS := $(patsubst %/,%,$(AUTO_TOOLS_SUBDIRS-y))

export OBC_TOP_DIR TOOLCHAIN_SYSROOT AUTO_TOOLS_OUTPUT_DIR

.PHONY: all build install clean show_config help

all: install

build: show_config
	@if [ -z "$(AUTO_TOOLS_SUBDIRS)" ]; then \
		echo "Warning: No auto-tools enabled in configuration."; \
		echo "Enable AUTO_TOOLS_* from system menuconfig."; \
	else \
		for dir in $(AUTO_TOOLS_SUBDIRS); do \
			echo "========================================"; \
			echo "Building $$dir..."; \
		$(MAKE) -C $$dir CROSS_COMPILE="$(CROSS_COMPILE)" \
			TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" || exit 1; \
		done; \
	fi

install: build
	@if [ -n "$(AUTO_TOOLS_SUBDIRS)" ]; then \
		mkdir -p "$(AUTO_TOOLS_OUTPUT_DIR)"; \
		for dir in $(AUTO_TOOLS_SUBDIRS); do \
			echo "Installing $$dir..."; \
			$(MAKE) -C $$dir install DESTDIR="$(AUTO_TOOLS_OUTPUT_DIR)" \
				CROSS_COMPILE="$(CROSS_COMPILE)" \
				TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" || exit 1; \
		done; \
		echo "Auto-tools installed in $(AUTO_TOOLS_OUTPUT_DIR)"; \
	else \
		echo "No auto-tools selected; nothing to install."; \
	fi

clean:
	@for dir in auto-mem auto-smart auto-priority; do \
		if [ -d $$dir ]; then $(MAKE) -C $$dir clean || exit 1; fi; \
	done
	@rm -rf "$(AUTO_TOOLS_OUTPUT_DIR)"

show_config:
	@echo "========================================"
	@echo "auto-tools Build Configuration"
	@echo "========================================"
	@echo "CROSS_COMPILE: $(CROSS_COMPILE)"
	@echo "OUTPUT_DIR:    $(AUTO_TOOLS_OUTPUT_DIR)"
	@echo "Enabled tools: $(if $(AUTO_TOOLS_SUBDIRS),$(AUTO_TOOLS_SUBDIRS),<none>)"
	@echo "========================================"

help:
	@echo "auto-tools are controlled by CONFIG_AUTO_TOOLS_* in .config."
	@echo "Use 'make menuconfig' from the OBC top directory to select them."
