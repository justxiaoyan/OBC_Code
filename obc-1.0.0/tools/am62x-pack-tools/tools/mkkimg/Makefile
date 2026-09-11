# Makefile for mkkimg tool

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
LDFLAGS =

TARGET = mkkimg
UMKKIMG = umkkimg
INSTALL_PATH ?= /usr/local/bin

# 源文件
SRCS = src/mkkimg.c src/crc.c
OBJS = $(SRCS:.c=.o)

UMKKIMG_SRCS = src/umkkimg.c src/crc.c
UMKKIMG_OBJS = $(UMKKIMG_SRCS:.c=.o)

# 默认目标
all: $(TARGET) $(UMKKIMG)

# 编译目标
$(TARGET): src/mkkimg.o src/crc.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "=========================================="
	@echo " Build complete: $(TARGET)"
	@echo "=========================================="

$(UMKKIMG): src/umkkimg.o src/crc.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "=========================================="
	@echo " Build complete: $(UMKKIMG)"
	@echo "=========================================="

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 安装
install: $(TARGET) $(UMKKIMG)
	install -m 755 $(TARGET) $(INSTALL_PATH)/
	install -m 755 $(UMKKIMG) $(INSTALL_PATH)/
	@echo "Installed to $(INSTALL_PATH)/"

# 清理
clean:
	rm -f $(TARGET) $(UMKKIMG) src/mkkimg.o src/umkkimg.o src/crc.o
	@echo "Clean complete"

# 测试
test: $(TARGET) $(UMKKIMG)
	@echo "Testing $(TARGET) and $(UMKKIMG)..."
	./$(TARGET) || true
	./$(UMKKIMG) || true

# 帮助
help:
	@echo "Makefile for mkkimg/umkkimg"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build mkkimg and umkkimg (default)"
	@echo "  install  - Install to $(INSTALL_PATH)"
	@echo "  clean    - Remove build files"
	@echo "  test     - Run basic test"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  CC            - C compiler (default: gcc)"
	@echo "  INSTALL_PATH  - Installation directory (default: /usr/local/bin)"

.PHONY: all install clean test help
