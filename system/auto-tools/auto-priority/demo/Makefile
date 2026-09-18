# Makefile for auto-priority_demo
# 线程优先级演示程序

# 交叉编译工具链
OBC_TOP_DIR ?= $(abspath $(CURDIR)/../../../..)
sinclude $(OBC_TOP_DIR)/.config
CROSS_COMPILE ?= $(subst ",,$(CONFIG_OBC_SDK_COMP))

# 编译器和工具
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

# 目标程序名
TARGET = auto-priority_demo

# 源文件
SRCS = priority_demo.c

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

# 生成strip版本（减小体积）
strip: $(TARGET)
	$(STRIP) $(TARGET)
	@echo "Stripped binary size:"
	@ls -lh $(TARGET)

# 清理编译生成的文件
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete"

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

# 帮助信息
help:
	@echo "Makefile for auto-priority_demo"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build the program (default)"
	@echo "  strip          - Build and strip the binary"
	@echo "  clean          - Remove build artifacts"
	@echo "  install        - Install to DESTDIR (requires DESTDIR=/path)"
	@echo "  help           - Show this help message"

.PHONY: all clean strip install help
