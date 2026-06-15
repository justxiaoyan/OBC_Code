# StockMonitor Makefile
# A股股市行情监控工具

# 编译器配置
CC = gcc

# 编译选项
CFLAGS = -Wall -O2
LDFLAGS = -lcjson

# 目标文件
TARGET = StockMonitor
SRC = StockMonitor.c

# 默认目标：本地编译
all: $(TARGET)
	@echo ""
	@echo "✅ 编译完成！"
	@echo "   平台: $(shell uname -s) $(shell uname -m)"
	@echo "   运行: ./$(TARGET) [股票代码...]"
	@echo ""

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# 清理
clean:
	rm -f $(TARGET) *.o
	@echo "清理完成"

# 运行（默认示例股票）
run: $(TARGET)
	@echo "启动 StockMonitor (按 Ctrl+C 停止)..."
	@echo ""
	./$(TARGET)

# 运行自定义股票
run-custom: $(TARGET)
	@echo "请输入股票代码（空格分隔）："
	@read codes; ./$(TARGET) $$codes

# 运行示例：贵州茅台
run-moutai: $(TARGET)
	./$(TARGET) sh600519

# 运行示例：多只股票
run-demo: $(TARGET)
	./$(TARGET) sh600519 sz000858 sh600036 sh600887

# 后台运行
start: $(TARGET)
	@if pgrep -x $(TARGET) > /dev/null; then \
		echo "❌ StockMonitor 已在运行中"; \
	else \
		nohup ./$(TARGET) > stock_monitor.log 2>&1 &\
		echo "✅ StockMonitor 已在后台启动 (PID: $$!)"; \
		echo "   日志文件: stock_monitor.log"; \
	fi

# 停止后台运行
stop:
	@if pgrep -x $(TARGET) > /dev/null; then \
		pkill -x $(TARGET) && echo "✅ StockMonitor 已停止"; \
	else \
		echo "❌ StockMonitor 未运行"; \
	fi

# 查看状态
status:
	@if pgrep -x $(TARGET) > /dev/null; then \
		echo "✅ StockMonitor 正在运行"; \
		ps aux | grep $(TARGET) | grep -v grep; \
	else \
		echo "❌ StockMonitor 未运行"; \
	fi

# 帮助信息
help:
	@echo "StockMonitor - A股股市行情监控工具"
	@echo ""
	@echo "编译和运行选项："
	@echo "  make              - 编译程序"
	@echo "  make run          - 运行（使用默认示例股票）"
	@echo "  make run-demo     - 运行多只股票示例"
	@echo "  make run-moutai   - 运行贵州茅台示例"
	@echo "  make start        - 在后台启动"
	@echo "  make stop         - 停止后台运行"
	@echo "  make status       - 查看运行状态"
	@echo "  make clean        - 清理编译产物"
	@echo ""
	@echo "股票代码格式："
	@echo "  上海股票: sh + 6位代码 (如: sh600519 贵州茅台)"
	@echo "  深圳股票: sz + 6位代码 (如: sz000858 五粮液)"
	@echo ""
	@echo "常见股票代码："
	@echo "  sh600519  贵州茅台"
	@echo "  sz000858  五粮液"
	@echo "  sh600036  招商银行"
	@echo "  sh600887  伊利股份"
	@echo "  sz000001  平安银行"
	@echo "  sh601318  中国平安"
	@echo ""

.PHONY: all clean run run-custom run-moutai run-demo start stop status help
