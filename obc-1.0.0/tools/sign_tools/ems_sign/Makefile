# Direct build helper for the EMS signing sources.
SOURCE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OUTPUT_DIR ?= $(SOURCE_DIR)/../output
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

.PHONY: all clean
all: $(OUTPUT_DIR)/ems_sign $(OUTPUT_DIR)/unsign_demo

$(OUTPUT_DIR):
	mkdir -p "$@"

$(OUTPUT_DIR)/ems_sign: $(SOURCE_DIR)/ems_sign.c $(SOURCE_DIR)/crc16.c $(SOURCE_DIR)/ems_sign.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) "$<" "$(SOURCE_DIR)/crc16.c" -o "$@"

$(OUTPUT_DIR)/unsign_demo: $(SOURCE_DIR)/unsign_demo.c $(SOURCE_DIR)/crc16.c $(SOURCE_DIR)/ems_sign.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) "$<" "$(SOURCE_DIR)/crc16.c" -o "$@"

clean:
	rm -rf "$(OUTPUT_DIR)"
