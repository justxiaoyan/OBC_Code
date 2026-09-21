OBC_TOP_DIR ?= $(abspath $(CURDIR)/../../..)
BUILD_DIR ?= $(OBC_TOP_DIR)/output/tmp/obctool-build
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
CFLAGS ?= -Os
CFLAGS += -Wall -Wextra -Werror
LDFLAGS ?=
TARGET := $(BUILD_DIR)/obctool

.PHONY: all install clean

all: $(TARGET)

$(TARGET): obctool.c
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(LDFLAGS) -o "$@" "$<"

install: all
	@test -n "$(DESTDIR)" || { echo "ERROR: DESTDIR is required"; exit 1; }
	install -D -m 0755 "$(TARGET)" "$(DESTDIR)/usr/obc/obctool"

clean:
	rm -rf "$(BUILD_DIR)"
