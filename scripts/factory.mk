FACTORY_IMAGE := $(call OBC_UPGRADE_IMAGE,factory)
LEGACY_FACTORY_IMAGE := $(OBC_PACK_IMAGE_DIR)/factory.bin
FACTORY_STAGING_DIR := $(OBC_PACK_DIR)/factory-input
ifeq ($(PLATFORM_NAME),imx6ull)
FACTORY_INPUTS := \
	$(call OBC_UPGRADE_IMAGE,fdt) \
	$(call OBC_UPGRADE_IMAGE,uboot) \
	$(call OBC_UPGRADE_IMAGE,kernel) \
	$(call OBC_UPGRADE_IMAGE,rootfs)
FACTORY_TARGETS := tools output uboot kernel rootfs
else
FACTORY_INPUTS := \
	$(call OBC_UPGRADE_IMAGE,loader) \
	$(call OBC_UPGRADE_IMAGE,fdt) \
	$(call OBC_UPGRADE_IMAGE,teeos) \
	$(call OBC_UPGRADE_IMAGE,uboot) \
	$(call OBC_UPGRADE_IMAGE,kernel) \
	$(call OBC_UPGRADE_IMAGE,rootfs)
FACTORY_TARGETS := tools output loader uboot kernel rootfs system module
endif

.PHONY: factory factory_clean
# Host tools are the global prerequisite for packaging. Their own Makefiles
# skip rebuilds when the source files have not changed.
.NOTPARALLEL: factory
ifneq ($(filter am62x imx6ull,$(PLATFORM_NAME)),)
factory: $(FACTORY_TARGETS)
	@set -eu; \
	if [ ! -x "$(MKKIMG)" ]; then \
		echo "ERROR: mkkimg tool not found: $(MKKIMG)"; \
		echo "       run 'make tools' first"; exit 1; \
	fi; \
	if [ ! -d "$(OBC_PACK_IMAGE_DIR)" ]; then \
		echo "ERROR: image directory not found: $(OBC_PACK_IMAGE_DIR)"; exit 1; \
	fi; \
	for input in $(FACTORY_INPUTS); do \
		if [ ! -f "$$input" ]; then \
			echo "ERROR: factory input not found: $$input"; exit 1; \
		fi; \
	done; \
	rm -rf "$(FACTORY_STAGING_DIR)"; \
	mkdir -p "$(FACTORY_STAGING_DIR)"; \
	for input in $(FACTORY_INPUTS); do \
		cp -a "$$input" "$(FACTORY_STAGING_DIR)/"; \
	done; \
	rm -f "$(LEGACY_FACTORY_IMAGE)"; \
	"$(MKKIMG)" "$(PLATFORM_NAME)" "$(FACTORY_STAGING_DIR)" "$(FACTORY_IMAGE)"
else
factory:
	@echo "ERROR: factory packaging is not implemented for $(PLATFORM_NAME)"; exit 1
endif

factory_clean:
	rm -rf "$(FACTORY_STAGING_DIR)"
	rm -f "$(FACTORY_IMAGE)" "$(LEGACY_FACTORY_IMAGE)"
