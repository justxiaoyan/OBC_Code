# Host-side factory image packer/unpacker.
TOOLS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OUTPUT_DIR ?= $(TOOLS_DIR)/output
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -I$(TOOLS_DIR)/include
LDFLAGS ?=
INSTALL_PATH ?=

MKKIMG := $(OUTPUT_DIR)/mkkimg
UMKKIMG := $(OUTPUT_DIR)/umkkimg

.PHONY: all clean install test help
all: $(MKKIMG) $(UMKKIMG)

$(OUTPUT_DIR):
	mkdir -p "$@"

$(OUTPUT_DIR)/mkkimg: $(TOOLS_DIR)/src/mkkimg.c $(TOOLS_DIR)/src/crc.c $(TOOLS_DIR)/include/mkkimg.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) "$<" "$(TOOLS_DIR)/src/crc.c" -o "$@"

$(OUTPUT_DIR)/umkkimg: $(TOOLS_DIR)/src/umkkimg.c $(TOOLS_DIR)/src/crc.c $(TOOLS_DIR)/include/mkkimg.h | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) "$<" "$(TOOLS_DIR)/src/crc.c" -o "$@"

install: all
	@test -n "$(INSTALL_PATH)" || { echo "ERROR: set INSTALL_PATH explicitly, for example make install INSTALL_PATH=/usr/local/bin"; exit 2; }
	install -m 755 "$(MKKIMG)" "$(INSTALL_PATH)/mkkimg"
	install -m 755 "$(UMKKIMG)" "$(INSTALL_PATH)/umkkimg"

clean:
	rm -rf "$(OUTPUT_DIR)"
	rm -f "$(TOOLS_DIR)/src"/*.o

test: all
	@test -x "$(MKKIMG)" -a -x "$(UMKKIMG)"

help:
	@echo "make [all|clean|test]"
	@echo "make install INSTALL_PATH=/path/to/bin"
