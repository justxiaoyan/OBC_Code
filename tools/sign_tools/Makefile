


# Host-side OBC signing tools.
TOOLS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE_DIR := $(TOOLS_DIR)/obc_sign
OUTPUT_DIR ?= $(TOOLS_DIR)/output
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

OBC_SIGN := $(OUTPUT_DIR)/obc_sign
UNSIGN_DEMO := $(OUTPUT_DIR)/unsign_demo

.PHONY: all clean install
all: $(OBC_SIGN) $(UNSIGN_DEMO)

$(OUTPUT_DIR):
	mkdir -p "$@"

$(OBC_SIGN): $(SOURCE_DIR)/obc_sign.c $(SOURCE_DIR)/crc16.c $(SOURCE_DIR)/obc_sign.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) "$<" "$(SOURCE_DIR)/crc16.c" -o "$@"

$(UNSIGN_DEMO): $(SOURCE_DIR)/unsign_demo.c $(SOURCE_DIR)/crc16.c $(SOURCE_DIR)/obc_sign.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) "$<" "$(SOURCE_DIR)/crc16.c" -o "$@"

install: all
	@echo "Installed signing tools in $(OUTPUT_DIR)"

clean:
	rm -rf "$(OUTPUT_DIR)"
	rm -f "$(SOURCE_DIR)"/*.o













