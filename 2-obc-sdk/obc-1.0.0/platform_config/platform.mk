# 定义支持的平台列表
PLATFORMS += imx6ull
PLATFORMS += platdemo
PLATFORM_COUNT := $(words $(PLATFORMS))



# 平台选择目标
platform:
	@echo "please choice platform (input number):"
	@i=1; \
	for plat in $(PLATFORMS); do \
		echo "  [$$i] $$plat"; \
		i=$$((i+1)); \
	done
	@read -p "input choice(1-$(PLATFORM_COUNT)): " CHOICE; \
	if [ -z "$$CHOICE" ]; then \
		echo "Error: no input !!!"; \
		exit 1; \
	elif ! echo "$$CHOICE" | grep -qE '^[0-9]+$$'; then \
		echo "Error: please input number !!!"; \
		exit 1; \
	elif [ "$$CHOICE" -lt 1 ] || [ "$$CHOICE" -gt $(PLATFORM_COUNT) ]; then \
		echo "Error: Invalid number !!!"; \
		exit 1; \
	else \
		i=1; \
		for plat in $(PLATFORMS); do \
			if [ "$$i" -eq "$$CHOICE" ]; then \
				if [ -f platform_config/$$plat/$$plat"_defconfig" ]; then \
					cp platform_config/$$plat/$$plat"_defconfig" .config; \
					echo "Success: cppied $$plat"_defconfig" to .config"; \
					exit 0; \
				else \
					echo "Error: not found platform_config/$$plat/$$plat"_defconfig" file"; \
					exit 1; \
				fi; \
			fi; \
			i=$$((i+1)); \
		done; \
	fi

	@$(MAKE) -C $(OBC_TOP_DIR) sdk_config

sdk_config:
		@cp platform_config/$(PLATFORM_CONFIG)/sdk_config/uboot-$(UBOOT_VERSION)-$(PLATFORM_CONFIG)-defconfig $(UBOOT_SDK_DIR)/.config
		@cp platform_config/$(PLATFORM_CONFIG)/sdk_config/kernel-$(KERNEL_VERSION)-$(PLATFORM_CONFIG)-defconfig $(KERNEL_SDK_DIR)/.config
		@echo "Success: cppied $(PLATFORM_CONFIG) sdk_config to .config"

saveconfig_uboot:
	@if [ -f $(UBOOT_SDK_DIR)/.config ]; then \
		cp $(UBOOT_SDK_DIR)/.config platform_config/$(PLATFORM_CONFIG)/sdk_config/uboot-$(UBOOT_VERSION)-$(PLATFORM_CONFIG)-defconfig; \
	else \
		echo "no such file $(UBOOT_SDK_DIR)/.config\r\n"; \
	fi

saveconfig_kernel:
	@if [ -f $(KERNEL_SDK_DIR)/.config ]; then \
		cp $(KERNEL_SDK_DIR)/.config platform_config/$(PLATFORM_CONFIG)/sdk_config/kernel-$(KERNEL_VERSION)-$(PLATFORM_CONFIG)-defconfig; \
	else \
		echo "no such file $(KERNEL_SDK_DIR)/.config\r\n"; \
	fi
