FACTORY_IMAGE := $(call OBC_UPGRADE_IMAGE,factory)
LEGACY_FACTORY_IMAGE := $(OBC_PACK_IMAGE_DIR)/factory.bin
FACTORY_INPUTS := \
	$(call OBC_UPGRADE_IMAGE,loader) \
	$(call OBC_UPGRADE_IMAGE,fdt) \
	$(call OBC_UPGRADE_IMAGE,teeos) \
	$(call OBC_UPGRADE_IMAGE,uboot) \
	$(call OBC_UPGRADE_IMAGE,kernel) \
	$(call OBC_UPGRADE_IMAGE,rootfs)

.PHONY: factory factory_clean
# Host tools are the global prerequisite for packaging. Their own Makefiles
# skip rebuilds when the source files have not changed.
.NOTPARALLEL: factory
factory: tools output loader uboot kernel rootfs system module
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
	rm -f "$(LEGACY_FACTORY_IMAGE)"; \
	"$(MKKIMG)" "$(PLATFORM_NAME)" "$(OBC_PACK_IMAGE_DIR)" "$(FACTORY_IMAGE)"

factory_clean:
	rm -f "$(FACTORY_IMAGE)" "$(LEGACY_FACTORY_IMAGE)"
