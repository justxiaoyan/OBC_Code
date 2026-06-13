
CC = arm-buildroot-linux-gnueabihf-gcc

.PHONY: all clean
all : pxp_demo

pxp_demo: pxp_demo.c
	@ $(CC) pxp_demo.c -o $@


install:
	cp pxp_demo $(OBC_OUTPUT_DIR) 


clean:
	-rm pxp_demo
	-rm $(OBC_OUTPUT_DIR)/pxp_demo
























