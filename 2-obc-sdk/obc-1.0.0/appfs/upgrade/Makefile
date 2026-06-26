# Makefile for EMS Upgrade Tool v2.0 (Refactored)

VERSION = 4.0.0

# 交叉编译工具链
CROSS_COMPILE ?= /opt/buildroot/aarch64-ca53-linux-gnu_sdk-buildroot/bin/aarch64-ca53-linux-gnu-

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -O2 -I./include -I../include -static
LDFLAGS = -static

TARGET = upgrade

# 源文件组织
COMMON_SRCS = common/partition.c \
              common/image.c \
              common/utils.c \
              common/upgrade.c \
              common/pkg_header.c

FS_SRCS = fs/fs_raw.c \
          fs/fs_ext4.c \
          fs/fs_ext4_sparse.c \
          fs/fs_squashfs.c \
          fs/fs_manager.c

MAIN_SRCS = src/main.c

SRCS = $(COMMON_SRCS) $(FS_SRCS) $(MAIN_SRCS)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "=========================================="
	@echo " Build complete: $(TARGET)"
	@echo " Architecture: ARM64 (aarch64)"
	@echo " Version: $(VERSION)"
	@echo "=========================================="

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
	@echo "Clean complete"

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/$(TARGET)"

install-nfs: $(TARGET)
	cp -v $(TARGET) /srv/nfs/hushanyan/
	@echo "Installed to /srv/nfs/hushanyan/$(TARGET)"

show-structure:
	@echo "Project Structure:"
	@echo "  common/   - Core modules (partition, image, upgrade, pkg_header)"
	@echo "  fs/       - Filesystem writers (raw, ext4, ext4-sparse, squashfs)"
	@echo "  src/      - Main entry point"
	@echo "  include/  - Header files"

.PHONY: all clean install install-nfs show-structure
