.PHONY: all clean install

# 编译选项
CFLAGS := -Wall -Wextra -O2

# 目标
all: ems_sign unsign_demo

# CRC16对象文件
crc16.o: crc16.c ems_sign.h
	gcc $(CFLAGS) -c crc16.c -o crc16.o

# ems_sign工具
ems_sign: ems_sign.c crc16.o ems_sign.h
	gcc $(CFLAGS) ems_sign.c crc16.o -o ems_sign

# unsign_demo工具
unsign_demo: unsign_demo.c crc16.o ems_sign.h
	gcc $(CFLAGS) unsign_demo.c crc16.o -o unsign_demo

# 安装到输出目录
install: all
	@if [ -n "$(OBC_OUTPUT_DIR)" ]; then \
		mv ems_sign $(OBC_OUTPUT_DIR)/; \
		mv unsign_demo $(OBC_OUTPUT_DIR)/; \
		echo "Installed to $(OBC_OUTPUT_DIR)"; \
	else \
		echo "Warning: OBC_OUTPUT_DIR not set"; \
	fi

# 清理
clean:
	-rm -f ems_sign unsign_demo crc16.o
	@if [ -n "$(OBC_OUTPUT_DIR)" ]; then \
		rm -f $(OBC_OUTPUT_DIR)/ems_sign; \
		rm -f $(OBC_OUTPUT_DIR)/unsign_demo; \
	fi
