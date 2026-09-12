# Makefile for auto-priority
# 线程优先级和CPU使用情况展示工具

# 交叉编译工具链
OBC_TOP_DIR ?= $(abspath $(CURDIR)/../../..)
sinclude $(OBC_TOP_DIR)/.config
CROSS_COMPILE ?= $(subst ",,$(CONFIG_OBC_SDK_COMP))

# 编译器和工具
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

# 目标程序名
TARGET = auto-priority
DEMO_DIR = demo
DEMO_TARGET = $(DEMO_DIR)/auto-priority_demo

# 源文件
SRCS = main.c

# 目标文件
OBJS = $(SRCS:.c=.o)

# 编译选项
SYSROOT_CFLAGS = $(if $(TOOLCHAIN_SYSROOT),--sysroot=$(TOOLCHAIN_SYSROOT),)
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE $(SYSROOT_CFLAGS)
LDFLAGS = -static -lpthread $(SYSROOT_CFLAGS)

# 默认目标
all: $(TARGET)

# 链接目标程序
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# 编译.c文件为.o文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译demo
demo:
	@echo "Building demo..."
	@$(MAKE) -C $(DEMO_DIR) CROSS_COMPILE=$(CROSS_COMPILE)
	@echo "Demo build complete: $(DEMO_TARGET)"

# 生成strip版本（减小体积）
strip: $(TARGET)
	$(STRIP) $(TARGET)
	@echo "Stripped binary size:"
	@ls -lh $(TARGET)

# Strip demo
strip-demo: demo
	@echo "Stripping demo..."
	@$(MAKE) -C $(DEMO_DIR) strip CROSS_COMPILE=$(CROSS_COMPILE)

# 清理编译生成的文件（包括demo）
clean:
	rm -f $(OBJS) $(TARGET)
	@$(MAKE) -C $(DEMO_DIR) clean
	@echo "Clean complete"

# 单独清理demo
clean-demo:
	@$(MAKE) -C $(DEMO_DIR) clean

# 安装到目标系统
install: $(TARGET)
	@if [ -z "$(DESTDIR)" ]; then \
		echo "Error: DESTDIR not set"; \
		echo "Usage: make install DESTDIR=/path/to/output"; \
		exit 1; \
	fi
	install -d $(DESTDIR)
	install -m 0755 $(TARGET) $(DESTDIR)/
	@echo "Installed to $(DESTDIR)/$(TARGET)"

# 安装demo
install-demo: demo
	@if [ -z "$(DESTDIR)" ]; then \
		echo "Error: DESTDIR not set"; \
		echo "Usage: make install-demo DESTDIR=/path/to/rootfs"; \
		exit 1; \
	fi
	install -d $(DESTDIR)
	install -m 0755 $(DEMO_TARGET) $(DESTDIR)/
	@echo "Installed to $(DESTDIR)/$(DEMO_TARGET)"

# 安装所有
install-all: install install-demo

# 帮助信息
help:
	@echo "Makefile for auto-priority"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build auto-priority (default)"
	@echo "  demo           - Build auto-priority_demo"
	@echo "  strip          - Build and strip auto-priority"
	@echo "  strip-demo     - Build and strip auto-priority_demo"
	@echo "  clean          - Remove all build artifacts (including demo)"
	@echo "  clean-demo     - Remove only demo build artifacts"
	@echo "  install        - Install auto-priority to DESTDIR"
	@echo "  install-demo   - Install auto-priority_demo to DESTDIR"
	@echo "  install-all    - Install both tools to DESTDIR"
	@echo "  help           - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE  - Cross compiler prefix"
	@echo "  DESTDIR        - Installation root directory"
	@echo ""
	@echo "Examples:"
	@echo "  make                                    # Build auto-priority"
	@echo "  make demo                               # Build auto-priority_demo"
	@echo "  make strip strip-demo                   # Build and strip both"
	@echo "  make clean                              # Clean everything"
	@echo "  make install-all DESTDIR=/mnt/rootfs    # Install both tools"

.PHONY: all demo strip strip-demo clean clean-demo install install-demo install-all help
