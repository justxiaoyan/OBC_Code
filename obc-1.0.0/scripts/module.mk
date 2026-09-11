include $(OBC_TOP_DIR)/module/module.mk
.PHONY: module module_clean
module: check_config output
	@if [ -z "$(MODULES)" ]; then echo "No modules enabled in configuration"; else \
		for mod in $(MODULES); do $(MAKE) -C "$(MODULE_DIR)/$$mod" all KERNEL_SDK_DIR="$(KERNEL_SDK_DIR)" CONFIG_OBC_SDK_ARCH="$(CONFIG_OBC_SDK_ARCH)" CONFIG_OBC_SDK_COMP="$(CONFIG_OBC_SDK_COMP)" MODULE_OUTPUT_DIR="$(MODULE_OUTPUT_DIR)" || exit 1; done; \
	fi
module_clean:
	@if [ -n "$(MODULES)" ]; then for mod in $(MODULES); do $(MAKE) -C "$(MODULE_DIR)/$$mod" clean || true; done; fi
	@rm -rf "$(MODULE_OUTPUT_DIR)"/*
