MODULE_NAME := obcbase
obj-m := $(MODULE_NAME).o

KDIR ?= $(KERNEL_SDK_DIR)
ARCH ?= $(if $(CONFIG_KERNEL_ARCH),$(patsubst "%",%,$(CONFIG_KERNEL_ARCH)),arm64)
CROSS_COMPILE ?= $(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
OBC_TOP_DIR ?= $(abspath $(CURDIR)/../..)
BUILD_DIR ?= $(OBC_TOP_DIR)/output/tmp/obcbase-build
MODULE_OUTPUT_DIR ?= $(OBC_TOP_DIR)/output/modules

.PHONY: all install clean

all:
	@test -d "$(KDIR)" || { echo "ERROR: kernel directory not found: $(KDIR)"; exit 1; }
	@mkdir -p "$(BUILD_DIR)"
	$(MAKE) -C "$(KDIR)" M="$(CURDIR)" MO="$(BUILD_DIR)" \
		ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" modules
	@test -f "$(CURDIR)/$(MODULE_NAME).ko" || { echo "ERROR: kernel did not produce $(MODULE_NAME).ko"; exit 1; }
	@cp "$(CURDIR)/$(MODULE_NAME).ko" "$(BUILD_DIR)/$(MODULE_NAME).ko"

install: all
	@test -f "$(BUILD_DIR)/$(MODULE_NAME).ko" || { echo "ERROR: $(MODULE_NAME).ko was not built"; exit 1; }
	@mkdir -p "$(MODULE_OUTPUT_DIR)"
	install -m 0644 "$(BUILD_DIR)/$(MODULE_NAME).ko" "$(MODULE_OUTPUT_DIR)/$(MODULE_NAME).ko"

clean:
	rm -rf "$(BUILD_DIR)"
