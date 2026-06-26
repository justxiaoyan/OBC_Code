################################################################################
# Makefile for AM62x Memory Statistics Tool
# AM62x 内存统计工具编译配置
################################################################################

# 交叉编译工具链支持
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
RM := rm -f

# 目录结构
SRC_DIR := src
DOC_DIR := doc
BUILD_DIR := build

# 编译选项
CFLAGS := -Wall -Wextra -O2 -std=c99 -I$(SRC_DIR)
LDFLAGS :=
LIBS :=

# 调试选项 (make DEBUG=1)
ifdef DEBUG
    CFLAGS += -g -DDEBUG
    CFLAGS := $(filter-out -O2,$(CFLAGS))
endif

# 目标程序
TARGET := mem_stat
TARGET_STATIC := mem_stat_static

# 源文件和目标文件
MAIN_SRC := main.c
LIB_SRCS := $(SRC_DIR)/sys_mem.c $(SRC_DIR)/user_mem.c $(SRC_DIR)/display.c
SRCS := $(MAIN_SRC) $(LIB_SRCS)

MAIN_OBJ := $(BUILD_DIR)/main.o
LIB_OBJS := $(LIB_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJS := $(MAIN_OBJ) $(LIB_OBJS)

# 头文件
HEADERS := $(wildcard $(SRC_DIR)/*.h)

################################################################################
# 编译规则
################################################################################

.PHONY: all clean install uninstall static help dirs

# 默认目标
all: dirs $(TARGET)

# 创建构建目录
dirs:
	@mkdir -p $(BUILD_DIR)

# 编译主程序
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $@"

# 编译静态链接版本
static: LDFLAGS += -static
static: dirs $(TARGET_STATIC)

$(TARGET_STATIC): $(OBJS)
	@echo "Linking $@ (static)..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $@"

# 编译目标文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 编译main.c (在根目录)
$(BUILD_DIR)/main.o: main.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

################################################################################
# 安装和清理
################################################################################

# 安装到系统
install: $(TARGET)
	@echo "Installing $(TARGET)..."
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installation complete."

# 卸载
uninstall:
	@echo "Uninstalling $(TARGET)..."
	$(RM) /usr/local/bin/$(TARGET)
	@echo "Uninstallation complete."

# 清理编译文件
clean:
	@echo "Cleaning..."
	$(RM) -r $(BUILD_DIR) $(TARGET) $(TARGET_STATIC)
	@echo "Clean complete."

################################################################################
# 测试和运行
################################################################################

# 运行程序
run: $(TARGET)
	./$(TARGET)

# 运行快速模式
run-quick: $(TARGET)
	./$(TARGET) -q

# 运行详细模式
run-verbose: $(TARGET)
	./$(TARGET) -v

# 生成报告
report: $(TARGET)
	./$(TARGET) > mem_report_$$(date +%Y%m%d_%H%M%S).txt
	@echo "Report generated."

################################################################################
# 代码检查
################################################################################

# 代码行数统计
count:
	@echo "Line count:"
	@wc -l $(SRCS) $(HEADERS) Makefile
	@echo ""
	@echo "Total C code:"
	@cat $(SRCS) $(HEADERS) | grep -v "^$$" | grep -v "^/\*" | grep -v "^ \*" | wc -l

# 代码格式化 (需要clang-format)
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Formatting code..."; \
		clang-format -i $(SRCS) $(HEADERS); \
		echo "Format complete."; \
	else \
		echo "clang-format not found. Skipping format."; \
	fi

# 项目结构
tree:
	@echo "Project structure:"
	@tree -L 2 || (echo "src/:" && ls -1 src/ && echo "doc/:" && ls -1 doc/)

################################################################################
# 帮助信息
################################################################################

help:
	@echo "AM62x Memory Statistics Tool - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build the program (default)"
	@echo "  static        - Build static linked version"
	@echo "  clean         - Remove compiled files"
	@echo "  install       - Install to /usr/local/bin"
	@echo "  uninstall     - Remove from /usr/local/bin"
	@echo "  run           - Build and run the program"
	@echo "  run-quick     - Run in quick mode"
	@echo "  run-verbose   - Run in verbose mode"
	@echo "  report        - Generate a report file"
	@echo "  count         - Count lines of code"
	@echo "  format        - Format code (requires clang-format)"
	@echo "  tree          - Show project structure"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1       - Build with debug symbols"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build release version"
	@echo "  make DEBUG=1            # Build debug version"
	@echo "  make clean all          # Clean and rebuild"
	@echo "  make static             # Build static version"
	@echo "  sudo make install       # Install to system"
	@echo ""
	@echo "Directory structure:"
	@echo "  src/          - Source code files"
	@echo "  doc/          - Documentation files"
	@echo "  build/        - Build artifacts (auto-created)"
