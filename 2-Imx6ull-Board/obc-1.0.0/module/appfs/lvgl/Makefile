#
# Makefile
#
#CC ?= gcc
CC = arm-none-linux-gnueabihf-gcc

# --- 新增：定义构建目录 ---
BUILD_DIR = ./build

LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
CFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
CFLAGS += -I$(LVGL_DIR)/platform/luckfox
CFLAGS += -I$(LVGL_DIR)/platform/luckfox/date
CFLAGS += -I$(LVGL_DIR)/platform/luckfox/sys_info
CFLAGS += -I$(LVGL_DIR)/lvgl
CFLAGS += -I$(LVGL_DIR)/lv_drivers/display
CFLAGS += -I$(LVGL_DIR)/lv_drivers/indev

LDFLAGS ?= -static -lm 
BIN = obc_lvgl

#Collect the files to compile
MAINSRC = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk

# --- 新增：引入 platform/luckfox 的 mk 文件 ---
include $(LVGL_DIR)/platform/luckfox/luckfox.mk

#CSRCS +=$(LVGL_DIR)/mouse_cursor_icon.c 

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

# --- 修改：将主程序目标文件也放入构建目录 ---
MAINOBJ = $(BUILD_DIR)/$(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)

# --- 修改：将所有目标文件的路径指向构建目录 ---
OBJS = $(addprefix $(BUILD_DIR)/,$(AOBJS) $(COBJS))

## MAINOBJ -> OBJFILES

all: default

# --- 修改：增加目录创建和路径重写的规则 ---
# 规则1: 编译 .c 文件到 $(BUILD_DIR) 中的 .o 文件
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $<"

# 规则2: 编译 .S 文件到 $(BUILD_DIR) 中的 .o 文件
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $<"
    
# --- 修改：确保最终可执行文件也在构建目录中 ---
default: $(BUILD_DIR)/$(BIN)

$(BUILD_DIR)/$(BIN): $(OBJS) $(MAINOBJ)
	@$(CC) -o $@ $(OBJS) $(MAINOBJ) $(LDFLAGS)

clean: 
# --- 修改：清理时直接删除整个构建目录 ---
	@rm -rf $(BUILD_DIR)