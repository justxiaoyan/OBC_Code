# AM62x A53 core 3 FreeRTOS AMP demo

# Use the OBC framework's configured compiler when invoked through system/appfs.
# Keep the standalone build default for users invoking this Makefile directly.
TOOLCHAIN_PREFIX ?= $(if $(strip $(CROSS_COMPILE)),$(CROSS_COMPILE),/home/shanyan-hu/toolchain/am62x-host/bin/aarch64-ca53-linux-gnu-)
TOOLCHAIN_ROOT := $(abspath $(dir $(TOOLCHAIN_PREFIX))/..)
CC      := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
SIZE    := $(TOOLCHAIN_PREFIX)size

BUILD_DIR := build/am62x
TARGET_ELF := $(BUILD_DIR)/rtos_app.elf
TARGET_BIN := $(BUILD_DIR)/rtos_app.bin
INSTALL_DIR ?= $(DESTDIR)
ifeq ($(strip $(INSTALL_DIR)),)
INSTALL_DIR := $(APPFS_OUTPUT_DIR)
endif

CPPFLAGS := -D__BUILD_A53_CORE3__
CPPFLAGS += -I. -Icsl -Iosal
CPPFLAGS += -Ikernel/freertos/config
CPPFLAGS += -Ikernel/freertos/FreeRTOS-Kernel/include
CPPFLAGS += -Ikernel/freertos/portable/GCC/a53
CPPFLAGS += -Iosal/soc/am62x

# Some Buildroot toolchains do not add their target libc headers when the
# compiler is invoked with -ffreestanding. Derive the sysroot from the chosen
# compiler so a different installed prefix remains usable.
TOOLCHAIN_TRIPLET := $(strip $(shell $(CC) -dumpmachine 2>/dev/null))
TOOLCHAIN_SYSROOT := $(strip $(shell $(CC) -print-sysroot 2>/dev/null))
ifneq ($(wildcard $(TOOLCHAIN_SYSROOT)/usr/include/stdlib.h),)
CPPFLAGS += -isystem $(TOOLCHAIN_SYSROOT)/usr/include
else ifneq ($(wildcard $(TOOLCHAIN_ROOT)/$(TOOLCHAIN_TRIPLET)/sysroot/usr/include/stdlib.h),)
CPPFLAGS += -isystem $(TOOLCHAIN_ROOT)/$(TOOLCHAIN_TRIPLET)/sysroot/usr/include
endif

CFLAGS := -mcpu=cortex-a53 -mabi=lp64 -mcmodel=large -mstrict-align
CFLAGS += -ffreestanding -fno-builtin -fdata-sections -ffunction-sections
CFLAGS += -std=gnu99 -Wall -Wextra -Wno-unused-parameter -g -O2 -fno-pie -fno-pic -fno-stack-protector
ASFLAGS := $(CFLAGS)
LDFLAGS := -Tmain_src/linker/linker_script.ld -nostdlib
# Ask the selected compiler where libgcc lives instead of assuming a vendor
# triplet or GCC version. GCC still supplies its normal sysroot include paths.
LIBGCC := $(strip $(shell $(CC) -print-libgcc-file-name 2>/dev/null))
LDLIBS := $(if $(LIBGCC),$(LIBGCC),-lgcc)

C_SRCS := \
	main_src/main.c main_src/libc_stubs.c \
	kernel/freertos/FreeRTOS-Kernel/timers.c \
	kernel/freertos/FreeRTOS-Kernel/queue.c \
	kernel/freertos/FreeRTOS-Kernel/list.c \
	kernel/freertos/FreeRTOS-Kernel/tasks.c \
	kernel/freertos/FreeRTOS-Kernel/event_groups.c \
	kernel/freertos/FreeRTOS-Kernel/stream_buffer.c \
	kernel/freertos/portable/MemMang/heap_4.c \
	kernel/freertos/portable/GCC/a53/port.c \
	kernel/freertos/portable/GCC/a53/port_Hwi.c \
	osal/src/freertos/HwiP_freertos.c \
	osal/src/freertos/LoadP_freertos.c \
	osal/src/freertos/MutexP_freertos.c \
	osal/src/freertos/SemaphoreP_freertos.c \
	osal/src/freertos/TaskP_freertos.c \
	osal/src/freertos/TimerP_freertos.c \
	osal/src/freertos/Utils_freertos.c \
	osal/src/nonos/DebugP_nonos.c \
	osal/src/nonos/RegisterIntr_nonos.c \
	osal/arch/core/a53/Arch_util.c \
	osal/soc/am62x/TimerP_default.c \
	csl/arch/a53/src/csl_a53.c \
	csl/arch/a53/src/interrupt.c \
	csl/arch/a53/src/startup/aarch64/csl_a53v8_startup.c \
	csl/src/ip/arm_gic/V3/Priv/csl_gic.c \
	csl/src/ip/timer/V0/Priv/timer.c

ASM_SRCS := \
	csl/arch/a53/src/csl_a53v8cache.asm \
	csl/arch/a53/src/csl_a53v8misc.asm \
	csl/arch/a53/src/startup/aarch64/bootcode.asm \
	kernel/freertos/portable/GCC/a53/portASM.asm \
	kernel/freertos/portable/GCC/a53/port_Hwi_asm_vectors.asm

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
OBJS += $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

.PHONY: all clean install

all: $(TARGET_ELF) $(TARGET_BIN)
	@$(SIZE) $(TARGET_ELF)
	@echo "Built $(TARGET_ELF) and $(TARGET_BIN) for AM62x A53 core 3"

install: all
	@test -n "$(INSTALL_DIR)" || { echo "ERROR: install output directory is not set"; exit 1; }
	@mkdir -p "$(INSTALL_DIR)"
	@install -m 0644 "$(TARGET_BIN)" "$(INSTALL_DIR)/rtos_app.bin"
	@install -m 0644 "$(TARGET_ELF)" "$(INSTALL_DIR)/rtos_app.elf"
	@echo "Installed rtos_app.bin and rtos_app.elf to $(INSTALL_DIR)"

$(TARGET_ELF): $(OBJS) main_src/linker/linker_script.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fno-pie -no-pie $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -x assembler-with-cpp -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
