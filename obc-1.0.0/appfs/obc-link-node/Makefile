# Makefile for OBC-Link Node

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu99 -I./include
LDFLAGS =

# 目录定义
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# 目标程序
TARGET = $(BIN_DIR)/obc-link-node

# 源文件和目标文件
SOURCES = $(wildcard $(SRC_DIR)/*.c)
MAIN_SRC = main.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
MAIN_OBJ = $(BUILD_DIR)/main.o
HEADERS = $(wildcard $(INC_DIR)/*.h)

# 默认目标
all: $(TARGET)

# 链接目标文件
$(TARGET): $(OBJECTS) $(MAIN_OBJ) | $(BIN_DIR)
	@echo "链接 $(TARGET)..."
	$(CC) $(OBJECTS) $(MAIN_OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "编译完成: $(TARGET)"

# 编译 src 目录下的源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	@echo "编译 $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 编译主程序
$(MAIN_OBJ): $(MAIN_SRC) $(HEADERS) | $(BUILD_DIR)
	@echo "编译 $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 创建必要的目录
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# 清理编译产物
clean:
	@echo "清理编译产物..."
	@rm -rf $(BUILD_DIR)/*.o
	@rm -rf $(BIN_DIR)/$(notdir $(TARGET))
	@echo "清理完成"

# 深度清理（包括目录）
distclean: clean
	@echo "深度清理..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "深度清理完成"

# 安装（可选）
install: $(TARGET)
	@echo "安装到 /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/obc-link-node
	@echo "安装完成"

# 卸载（可选）
uninstall:
	@echo "卸载..."
	@sudo rm -f /usr/local/bin/obc-link-node
	@echo "卸载完成"

# 运行
run: $(TARGET)
	@$(TARGET)

# 调试编译
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# 帮助信息
help:
	@echo "可用的 make 目标:"
	@echo "  all        - 编译程序 (默认)"
	@echo "  clean      - 清理编译产物 (.o 文件和可执行文件)"
	@echo "  distclean  - 深度清理 (删除 build 和 bin 目录)"
	@echo "  run        - 编译并运行程序"
	@echo "  debug      - 编译调试版本"
	@echo "  install    - 安装到系统 (/usr/local/bin)"
	@echo "  uninstall  - 从系统卸载"
	@echo "  help       - 显示此帮助信息"
	@echo ""
	@echo "项目结构:"
	@echo "  src/       - 源文件 (.c)"
	@echo "  include/   - 头文件 (.h)"
	@echo "  build/     - 编译产物 (.o)"
	@echo "  bin/       - 可执行文件"

.PHONY: all clean distclean install uninstall run debug help
