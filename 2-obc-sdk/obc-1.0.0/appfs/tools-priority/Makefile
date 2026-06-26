# Makefile for priority_dump
# 线程优先级和CPU使用情况展示工具

# 交叉编译工具链
CROSS_COMPILE ?= /home/ayan-server/HDD-B/justxiaoyan-github/AM62x/TI_SDK/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-

# 编译器和工具
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

# 目标程序名
TARGET = priority_dump
DEMO_DIR = demo
DEMO_TARGET = $(DEMO_DIR)/priority_demo

# 源文件
SRCS = priority_dump.c

# 目标文件
OBJS = $(SRCS:.c=.o)

# 编译选项
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE
LDFLAGS = -static -lpthread

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
		echo "Usage: make install DESTDIR=/path/to/rootfs"; \
		exit 1; \
	fi
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
	@echo "Installed to $(DESTDIR)/usr/bin/$(TARGET)"

# 安装demo
install-demo: demo
	@if [ -z "$(DESTDIR)" ]; then \
		echo "Error: DESTDIR not set"; \
		echo "Usage: make install-demo DESTDIR=/path/to/rootfs"; \
		exit 1; \
	fi
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(DEMO_TARGET) $(DESTDIR)/usr/bin/
	@echo "Installed to $(DESTDIR)/usr/bin/priority_demo"

# 安装所有
install-all: install install-demo

# 帮助信息
help:
	@echo "Makefile for priority_dump"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build priority_dump (default)"
	@echo "  demo           - Build priority_demo"
	@echo "  strip          - Build and strip priority_dump"
	@echo "  strip-demo     - Build and strip priority_demo"
	@echo "  clean          - Remove all build artifacts (including demo)"
	@echo "  clean-demo     - Remove only demo build artifacts"
	@echo "  install        - Install priority_dump to DESTDIR"
	@echo "  install-demo   - Install priority_demo to DESTDIR"
	@echo "  install-all    - Install both tools to DESTDIR"
	@echo "  help           - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE  - Cross compiler prefix"
	@echo "  DESTDIR        - Installation root directory"
	@echo ""
	@echo "Examples:"
	@echo "  make                                    # Build priority_dump"
	@echo "  make demo                               # Build priority_demo"
	@echo "  make strip strip-demo                   # Build and strip both"
	@echo "  make clean                              # Clean everything"
	@echo "  make install-all DESTDIR=/mnt/rootfs    # Install both tools"

.PHONY: all demo strip strip-demo clean clean-demo install install-demo install-all help
