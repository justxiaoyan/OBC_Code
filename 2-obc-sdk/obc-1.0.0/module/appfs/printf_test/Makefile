
CC = arm-buildroot-linux-gnueabihf-gcc

.PHONY: all clean
all : printf_test

printf_test: printf_test.c
	@ $(CC) printf_test.c -o $@


install:
	cp printf_test $(OBC_OUTPUT_DIR) 


clean:
	-rm printf_test
	-rm $(OBC_OUTPUT_DIR)/printf_test
























