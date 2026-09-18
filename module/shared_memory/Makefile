#
#  Creater liuzhengwu 20230206
#  Modified for OBC build system
#

# Module name
MODULE_NAME := multicore-shared-memory
obj-m := $(MODULE_NAME).o

# Get kernel directory from OBC build system
KDIR ?= $(KERNEL_SDK_DIR)

# Cross-compile configuration
ARCH ?= $(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH))
CROSS_COMPILE ?= $(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))

# Output directory
MODULE_OUTPUT_DIR ?= $(PWD)/../../output/modules

PWD := $(shell pwd)

.PHONY: all clean install

all:
	@if [ -z "$(KDIR)" ] || [ ! -d "$(KDIR)" ]; then \
		echo "Error: Kernel directory not found: $(KDIR)"; \
		echo "Please check your platform configuration"; \
		exit 1; \
	fi
	@echo "Building $(MODULE_NAME) module..."
	@echo "  KDIR: $(KDIR)"
	@echo "  ARCH: $(ARCH)"
	@echo "  CROSS_COMPILE: $(CROSS_COMPILE)"
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	@echo "Cleaning $(MODULE_NAME) module..."
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	@rm -rf *.o *.ko *.o.cmd *.mod.c *.mod *.symvers *.order .*.cmd .tmp_versions modules.order Module.symvers

install:
	@if [ ! -d "$(MODULE_OUTPUT_DIR)" ]; then \
		mkdir -p $(MODULE_OUTPUT_DIR); \
	fi
	@if [ -f $(MODULE_NAME).ko ]; then \
		cp $(MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/; \
		echo "  Installed: $(MODULE_NAME).ko to $(MODULE_OUTPUT_DIR)"; \
	else \
		echo "  Warning: $(MODULE_NAME).ko not found"; \
	fi
