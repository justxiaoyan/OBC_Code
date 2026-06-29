# LiteMonitor Makefile
# 说明：LiteMonitor 在 PC/Ubuntu 平台运行，监控本机系统状态并通过 UDP 广播

# 编译器配置
CC = gcc

# 编译选项
CFLAGS = -Wall -O2
LDFLAGS = -lpthread

# 目标文件
TARGET = LiteMonitor
SRC = LiteMonitor.c

# 默认目标：本地编译（Ubuntu/Linux PC）
all: $(TARGET)
	@echo ""
	@echo "✅ 编译完成！"
	@echo "   平台: $(shell uname -s) $(shell uname -m)"
	@echo "   运行: ./$(TARGET)"
	@echo ""

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# 清理
clean:
	rm -f $(TARGET) *.o
	@echo "清理完成"

# 运行
run: $(TARGET)
	@echo "启动 LiteMonitor (按 Ctrl+C 停止)..."
	@echo ""
	./$(TARGET)

# 后台运行
start: $(TARGET)
	@if pgrep -x $(TARGET) > /dev/null; then \
		echo "❌ LiteMonitor 已在运行中"; \
	else \
		nohup ./$(TARGET) > /dev/null 2>&1 &\
		echo "✅ LiteMonitor 已在后台启动 (PID: $$!)"; \
	fi

# 停止后台运行
stop:
	@if pgrep -x $(TARGET) > /dev/null; then \
		pkill -x $(TARGET) && echo "✅ LiteMonitor 已停止"; \
	else \
		echo "❌ LiteMonitor 未运行"; \
	fi

# 查看状态
status:
	@if pgrep -x $(TARGET) > /dev/null; then \
		echo "✅ LiteMonitor 正在运行"; \
		ps aux | grep $(TARGET) | grep -v grep; \
	else \
		echo "❌ LiteMonitor 未运行"; \
	fi

# 测试 UDP 广播（需要安装 netcat）
test-udp:
	@echo "监听 UDP 广播 (端口 5005)..."
	@echo "按 Ctrl+C 停止"
	@nc -ul 5005 | od -A x -t x1z -v

# 帮助信息
help:
	@echo "LiteMonitor 编译和运行选项："
	@echo ""
	@echo "  make          - 编译 LiteMonitor"
	@echo "  make run      - 编译并运行（前台）"
	@echo "  make start    - 在后台启动"
	@echo "  make stop     - 停止后台运行"
	@echo "  make status   - 查看运行状态"
	@echo "  make clean    - 清理编译产物"
	@echo "  make test-udp - 测试 UDP 广播接收"
	@echo ""
	@echo "配置修改："
	@echo "  编辑 LiteMonitor.c 修改广播地址和端口："
	@echo "    #define BROADCAST_IP \"10.10.0.255\""
	@echo "    #define BROADCAST_PORT 5005"
	@echo ""

.PHONY: all clean run start stop status test-udp help
