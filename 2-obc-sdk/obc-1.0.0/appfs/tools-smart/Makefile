CROSS_COMPILE ?= /home/ayan-server/HDD-B/justxiaoyan-github/AM62x/TI_SDK/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-
CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

TARGET := tools-smart
BUILD_DIR := build

CFLAGS ?= -O2 -Wall -Wextra -Werror
CPPFLAGS += -Ivendor
LDFLAGS ?=

SRCS := src/main.c \
        vendor/generic/generic.c \
        vendor/longsys/longsys.c \
        vendor/micron/micron.c \
        vendor/samsung/samsung.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean strip help

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

help:
	@printf 'Targets:\n'
	@printf '  make              Build smart_monitor\n'
	@printf '  make clean        Remove build output\n'
	@printf '  make strip        Strip target binary\n'
	@printf '\nConfiguration:\n'
	@printf '  CROSS_COMPILE=%s\n' '$(CROSS_COMPILE)'
	@printf '  CC=%s\n' '$(CC)'

-include $(DEPS)
