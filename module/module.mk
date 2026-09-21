# Module configuration

# Module build directory
MODULE_DIR := $(OBC_TOP_DIR)/module
MODULE_OUTPUT_DIR := $(OBC_OUTPUT_DIR)/modules

# List of available modules
MODULES :=

ifeq ($(CONFIG_MODULE_SHARED_MEMORY),y)
	MODULES += shared_memory
endif

ifeq ($(CONFIG_OBC_BASE),y)
	MODULES += obcbase
endif

# Export variables for module Makefiles
export MODULE_DIR
export MODULE_OUTPUT_DIR
