# Makefile for priority_demo
# 线程优先级演示程序

# 交叉编译工具链
CROSS_COMPILE ?= /home/ayan-server/HDD-B/justxiaoyan-github/AM62x/TI_SDK/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-

# 编译器和工具
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

# 目标程序名
TARGET = priority_demo

# 源文件
SRCS = priority_demo.c

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
		echo "Usage: make install DESTDIR=/path/to/rootfs"; \
		exit 1; \
	fi
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
	@echo "Installed to $(DESTDIR)/usr/bin/$(TARGET)"

# 帮助信息
help:
	@echo "Makefile for priority_demo"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build the program (default)"
	@echo "  strip          - Build and strip the binary"
	@echo "  clean          - Remove build artifacts"
	@echo "  install        - Install to DESTDIR (requires DESTDIR=/path)"
	@echo "  help           - Show this help message"

.PHONY: all clean strip install help
