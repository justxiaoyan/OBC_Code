OBC_TOP_DIR ?= $(abspath $(CURDIR)/../../..)
sinclude $(OBC_TOP_DIR)/.config
CROSS_COMPILE ?= $(subst ",,$(CONFIG_OBC_SDK_COMP))
CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

TARGET := auto-smart
BUILD_DIR := build

CFLAGS ?= -O2 -Wall -Wextra -Werror
SYSROOT_CFLAGS := $(if $(TOOLCHAIN_SYSROOT),--sysroot=$(TOOLCHAIN_SYSROOT),)
override CFLAGS += $(SYSROOT_CFLAGS)
CPPFLAGS += -Ivendor
LDFLAGS ?= $(SYSROOT_CFLAGS)

SRCS := src/main.c \
        vendor/generic/generic.c \
        vendor/longsys/longsys.c \
        vendor/micron/micron.c \
        vendor/samsung/samsung.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean strip install help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

strip: $(TARGET)
	$(STRIP) $(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
	@if [ -z "$(DESTDIR)" ]; then \
		echo "Error: DESTDIR not set"; \
		exit 1; \
	fi
	install -d $(DESTDIR)
	install -m 755 $(TARGET) $(DESTDIR)/

help:
	@printf 'Targets:\n'
	@printf '  make              Build auto-smart\n'
	@printf '  make clean        Remove build output\n'
	@printf '  make strip        Strip target binary\n'
	@printf '\nConfiguration:\n'
	@printf '  CROSS_COMPILE=%s\n' '$(CROSS_COMPILE)'
	@printf '  CC=%s\n' '$(CC)'

-include $(DEPS)
