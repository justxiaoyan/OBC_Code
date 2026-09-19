################################################################################
# system top-level autobuild dispatcher
################################################################################

OBC_TOP_DIR ?= $(abspath $(CURDIR)/..)
sinclude $(OBC_TOP_DIR)/.config

CROSS_COMPILE ?= $(subst ",,$(CONFIG_OBC_SDK_COMP))
TOOLCHAIN_SYSROOT ?= $(shell gcc=$(shell command -v $(CROSS_COMPILE)gcc 2>/dev/null); \
	root=$$(cd "$$(dirname "$$gcc")/.." 2>/dev/null && pwd); \
	triple=$(patsubst %-,%,$(CROSS_COMPILE)); \
	for candidate in "$$root/$$triple/sysroot" "$$root"/*/sysroot; do \
		if [ -d "$$candidate" ]; then echo "$$candidate"; break; fi; \
	done)

SYSTEM_OUTPUT_DIR := $(OBC_TOP_DIR)/output
SYSTEM_CHILD_DIRS := $(sort $(patsubst %/,%,$(wildcard */)))
SYSTEM_AUTOBUILD_DIRS := $(sort $(patsubst %/autobuild.sh,%,$(wildcard */autobuild.sh)))

export OBC_TOP_DIR CROSS_COMPILE TOOLCHAIN_SYSROOT SYSTEM_OUTPUT_DIR

.PHONY: all install clean show_config \
	run-autobuild run-autobuild-install run-autobuild-clean help

all: show_config run-autobuild

install: show_config run-autobuild-install
	@echo "System components installed under $(SYSTEM_OUTPUT_DIR)"

run-autobuild:
	@if [ -z "$(SYSTEM_AUTOBUILD_DIRS)" ]; then \
		echo "No system autobuild components found."; \
	else \
		for dir in $(SYSTEM_AUTOBUILD_DIRS); do \
			echo "Autobuild system/$$dir..."; \
			( cd "$$dir" && \
			  OBC_TOP_DIR="$(OBC_TOP_DIR)" SYSTEM_OUTPUT_DIR="$(SYSTEM_OUTPUT_DIR)" \
			  DESTDIR="$(SYSTEM_OUTPUT_DIR)/$$dir" CROSS_COMPILE="$(CROSS_COMPILE)" \
			  TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" sh ./autobuild.sh ) || exit 1; \
		done; \
	fi

run-autobuild-install:
	@if [ -z "$(SYSTEM_AUTOBUILD_DIRS)" ]; then \
		echo "No system autobuild components to install."; \
	else \
		for dir in $(SYSTEM_AUTOBUILD_DIRS); do \
			echo "Autobuild and install system/$$dir..."; \
			( cd "$$dir" && \
			  OBC_TOP_DIR="$(OBC_TOP_DIR)" SYSTEM_OUTPUT_DIR="$(SYSTEM_OUTPUT_DIR)" \
			  DESTDIR="$(SYSTEM_OUTPUT_DIR)/$$dir" CROSS_COMPILE="$(CROSS_COMPILE)" \
			  TOOLCHAIN_SYSROOT="$(TOOLCHAIN_SYSROOT)" sh ./autobuild.sh install ) || exit 1; \
		done; \
	fi

run-autobuild-clean:
	@for dir in $(SYSTEM_AUTOBUILD_DIRS); do \
		echo "Cleaning system/$$dir..."; \
		( cd "$$dir" && sh ./autobuild.sh clean ) || exit 1; \
	done

clean: show_config run-autobuild-clean
	@rm -rf "$(SYSTEM_OUTPUT_DIR)/appfs" "$(SYSTEM_OUTPUT_DIR)/auto-tools" "$(SYSTEM_OUTPUT_DIR)/third-part"
	@echo "System clean complete."

show_config:
	@echo "========================================"
	@echo "system Build Configuration"
	@echo "========================================"
	@echo "CROSS_COMPILE: $(CROSS_COMPILE)"
	@echo "OUTPUT_DIR:    $(SYSTEM_OUTPUT_DIR)"
	@echo "Child dirs:    $(if $(SYSTEM_CHILD_DIRS),$(SYSTEM_CHILD_DIRS),<none>)"
	@echo "Autobuild dirs:$(if $(SYSTEM_AUTOBUILD_DIRS),$(SYSTEM_AUTOBUILD_DIRS),<none>)"
	@echo "========================================"

help:
	@echo "system Makefile - recursively dispatches autobuild.sh components"
	@echo "Every direct child component must provide autobuild.sh."
