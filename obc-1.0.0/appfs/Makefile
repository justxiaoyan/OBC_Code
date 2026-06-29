################################################################################
# appfs Makefile - 应用程序文件系统编译配置
################################################################################

# 获取顶层目录和配置
OBC_TOP_DIR ?= $(shell cd .. && pwd)
sinclude $(OBC_TOP_DIR)/.config

# 从 CONFIG_ 变量中提取实际值（去掉引号）
CROSS_COMPILE := $(patsubst "%",%,$(CONFIG_OBC_SDK_COMP))
ARCH := $(patsubst "%",%,$(CONFIG_OBC_SDK_ARCH))

# 输出目录
APPFS_OUTPUT_DIR := $(OBC_TOP_DIR)/output/appfs
export APPFS_OUTPUT_DIR

################################################################################
# 根据 CONFIG 配置定义需要编译的子目录
################################################################################

# 定义所有可能的应用程序目录
APPFS_SUBDIRS-$(CONFIG_APPFS_TOOLS_MEM)      += tools-mem
APPFS_SUBDIRS-$(CONFIG_APPFS_TOOLS_SMART)    += tools-smart
APPFS_SUBDIRS-$(CONFIG_APPFS_TOOLS_PRIORITY) += tools-priority
APPFS_SUBDIRS-$(CONFIG_APPFS_LITE_MONITOR)   += LiteMonitor
APPFS_SUBDIRS-$(CONFIG_APPFS_STOCK_MONITOR)  += StockMonitor
APPFS_SUBDIRS-$(CONFIG_APPFS_LVGL)           += lvgl
APPFS_SUBDIRS-$(CONFIG_APPFS_CAMERA_FB)      += camera-fb
APPFS_SUBDIRS-$(CONFIG_APPFS_PXP_DEMO)       += pxp_demo
APPFS_SUBDIRS-$(CONFIG_APPFS_PRINTF_TEST)    += printf_test
APPFS_SUBDIRS-$(CONFIG_APPFS_UPGRADE)        += upgrade

# 过滤出启用的子目录 (CONFIG_xxx=y)
APPFS_SUBDIRS := $(patsubst %/,%,$(APPFS_SUBDIRS-y))

################################################################################
# 编译目标
################################################################################

.PHONY: all clean install help show_config

all: show_config
	@if [ -z "$(APPFS_SUBDIRS)" ]; then \
		echo "Warning: No appfs applications enabled in configuration."; \
		echo "Please check .config file or run 'make menuconfig' to enable applications."; \
	else \
		mkdir -p $(APPFS_OUTPUT_DIR); \
		echo "Building appfs applications..."; \
		for dir in $(APPFS_SUBDIRS); do \
			if [ -d $$dir ]; then \
				echo ""; \
				echo "========================================"; \
				echo "Building $$dir..."; \
				echo "========================================"; \
				$(MAKE) -C $$dir CROSS_COMPILE=$(CROSS_COMPILE) || exit 1; \
			else \
				echo "Warning: Directory $$dir not found, skipping..."; \
			fi; \
		done; \
		echo ""; \
		echo "========================================"; \
		echo "All appfs applications built successfully!"; \
		echo "========================================"; \
	fi

install: all
	@echo "Installing appfs applications to $(APPFS_OUTPUT_DIR)..."
	@mkdir -p $(APPFS_OUTPUT_DIR)
	@for dir in $(APPFS_SUBDIRS); do \
		if [ -d $$dir ]; then \
			echo "Installing $$dir..."; \
			if [ -f $$dir/Makefile ]; then \
				$(MAKE) -C $$dir install DESTDIR=$(APPFS_OUTPUT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) || true; \
			fi; \
			find $$dir -maxdepth 2 -type f -executable ! -name "*.sh" ! -name "*.py" ! -name "Makefile" -exec sh -c 'file {} | grep -q "ELF.*executable" && cp {} $(APPFS_OUTPUT_DIR)/' \; 2>/dev/null || true; \
		fi; \
	done
	@echo ""
	@echo "Installation complete. Installed files:"
	@ls -lh $(APPFS_OUTPUT_DIR)/ | grep "^-" | awk '{print "  " $$9 " (" $$5 ")"}'
	@echo ""
	@echo "Installation directory: $(APPFS_OUTPUT_DIR)"

clean:
	@echo "Cleaning appfs applications..."
	@for dir in $(APPFS_SUBDIRS); do \
		if [ -d $$dir ]; then \
			echo "Cleaning $$dir..."; \
			$(MAKE) -C $$dir clean 2>/dev/null || true; \
		fi; \
	done
	@rm -rf $(APPFS_OUTPUT_DIR)
	@echo "Clean complete."

show_config:
	@echo "========================================"
	@echo "appfs Build Configuration"
	@echo "========================================"
	@echo "CROSS_COMPILE: $(CROSS_COMPILE)"
	@echo "ARCH:          $(ARCH)"
	@echo "OUTPUT_DIR:    $(APPFS_OUTPUT_DIR)"
	@echo "Enabled apps:  $(if $(APPFS_SUBDIRS),$(APPFS_SUBDIRS),<none>)"
	@echo "========================================"

help:
	@echo "appfs Makefile - Application File System Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all enabled applications (default)"
	@echo "  install      - Install built applications to output directory"
	@echo "  clean        - Clean all built applications"
	@echo "  show_config  - Display current build configuration"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  Applications are controlled by CONFIG_APPFS_* variables in .config"
	@echo "  Use 'make menuconfig' in the top directory to configure."
	@echo ""
	@echo "Currently enabled applications:"
	@if [ -z "$(APPFS_SUBDIRS)" ]; then \
		echo "  <none>"; \
	else \
		for dir in $(APPFS_SUBDIRS); do \
			echo "  - $$dir"; \
		done; \
	fi
