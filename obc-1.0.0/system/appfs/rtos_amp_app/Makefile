# ==============================================================================
# 最终完整版 Makefile
# 1. 包含 FreeRTOS 核心头文件
# 2. 包含 FreeRTOS 配置文件 (FreeRTOSConfig.h)
# 3. 包含 FreeRTOS 移植层头文件
# 4. 移除架构冲突警告
# ==============================================================================

# --- 1. 工具链配置 ---
TOOLCHAIN_PREFIX := aarch64-none-elf-
CC      := $(TOOLCHAIN_PREFIX)gcc
LD      := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
SIZE    := $(TOOLCHAIN_PREFIX)size

# --- 2. 目录与核心配置 ---
SRC_DIR     := main_src
BUILD_DIR   := build/main
LIB_DIR     := build/rtos_lib
LINKER_DIR  := main_src/linker
CORE_ID     := 5

# --- 3. 编译标志 ---
CFLAGS  := -mcpu=cortex-a55 -g -O2 -Wall
CFLAGS  += -ffreestanding -nostdlib -fno-builtin

# 核心宏定义
CFLAGS += -D__BUILD_A55_CORE$(CORE_ID)__

# 包含路径
CFLAGS  += -I$(SRC_DIR)
CFLAGS  += -I. 
CFLAGS  += -Icsl
CFLAGS  += -Icsl/src/ip/timer/V0
CFLAGS  += -Iosal/include

# 【新增】FreeRTOS 配置路径 (包含 FreeRTOSConfig.h)
CFLAGS  += -Ikernel/freertos/config

# FreeRTOS 核心路径 (包含 FreeRTOS.h)
CFLAGS  += -Ikernel/freertos/FreeRTOS-Kernel/include

# FreeRTOS Portable 路径
CFLAGS  += -Ikernel/freertos/FreeRTOS-Kernel/portable/GCC/ARM_CA55_64_BIT

CFLAGS  += -Ikernel/freertos/portable/GCC/a55
CFLAGS  += -Ikernel/freertos/FreeRTOS-Kernel/vs_uart

# 汇编标志
ASFLAGS := -mcpu=cortex-a55 -g
ASFLAGS += -D__BUILD_A55_CORE$(CORE_ID)__

# --- 4. 链接标志 ---
LDFLAGS := -T$(LINKER_DIR)/linker_script.ld
LDFLAGS += --specs=nosys.specs
LDFLAGS += -L$(LIB_DIR)
# 【修正点】使用 -l:filename.a 语法来链接没有 'lib' 前缀的库文件
LDFLAGS += -Wl,--start-group
LDFLAGS += -l:freertos_a55.a
LDFLAGS += -l:libosal_a55.a   # 这个有 lib 前缀，用 -l: 也可以，或者用 -losal_a55
LDFLAGS += -l:libcsl_a55.a    # 这个有 lib 前缀
LDFLAGS += -Wl,--end-group
LDFLAGS += -lc -lgcc

# --- 5. 源文件与目标 ---
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TARGET_ELF := $(BUILD_DIR)/rtos_app.elf
TARGET_BIN := $(BUILD_DIR)/rtos_app.bin

# --- 6. 构建规则 ---

.PHONY: all clean

all: $(TARGET_ELF) $(TARGET_BIN)
	@echo "=========================================="
	@echo "Build successful for Core $(CORE_ID)!"
	@$(SIZE) $(TARGET_ELF)

$(TARGET_ELF): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "Linking $@ ..."
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_BIN): $(TARGET_ELF)
	@echo "Generating binary..."
	@$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $< ..."
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean done."