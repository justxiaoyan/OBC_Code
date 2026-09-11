


# Host-side EMS signing tools.
TOOLS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE_DIR := $(TOOLS_DIR)/ems_sign
OUTPUT_DIR ?= $(TOOLS_DIR)/output
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

EMS_SIGN := $(OUTPUT_DIR)/ems_sign
UNSIGN_DEMO := $(OUTPUT_DIR)/unsign_demo

.PHONY: all clean install
all: $(EMS_SIGN) $(UNSIGN_DEMO)

$(OUTPUT_DIR):
	mkdir -p "$@"

$(EMS_SIGN): $(SOURCE_DIR)/ems_sign.c $(SOURCE_DIR)/crc16.c $(SOURCE_DIR)/ems_sign.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) "$<" "$(SOURCE_DIR)/crc16.c" -o "$@"

$(UNSIGN_DEMO): $(SOURCE_DIR)/unsign_demo.c $(SOURCE_DIR)/crc16.c $(SOURCE_DIR)/ems_sign.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) "$<" "$(SOURCE_DIR)/crc16.c" -o "$@"

install: all
	@echo "Installed signing tools in $(OUTPUT_DIR)"

clean:
	rm -rf "$(OUTPUT_DIR)"
	rm -f "$(SOURCE_DIR)"/*.o














