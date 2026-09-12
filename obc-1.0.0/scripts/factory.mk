FACTORY_IMAGE := $(OBC_PACK_IMAGE_DIR)/factory.bin
FACTORY_INPUTS := \
	$(OBC_PACK_IMAGE_DIR)/100p-loader.bin \
	$(OBC_PACK_IMAGE_DIR)/100p-fdt.bin \
	$(OBC_PACK_IMAGE_DIR)/100p-teeos.bin \
	$(OBC_PACK_IMAGE_DIR)/100p-uboot.bin \
	$(OBC_PACK_IMAGE_DIR)/100p-kernel.bin \
	$(OBC_PACK_IMAGE_DIR)/100p-rootfs.bin

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
	"$(MKKIMG)" "$(OBC_PACK_IMAGE_DIR)" "$(FACTORY_IMAGE)"

factory_clean:
	rm -f "$(FACTORY_IMAGE)"
