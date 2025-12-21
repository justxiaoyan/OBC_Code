
CC = arm-buildroot-linux-gnueabihf-gcc

.PHONY: all clean
all : camera-fb

camera-fb: camera-fb.c
	@ $(CC) camera-fb.c -o $@


install:
	cp camera-fb $(OBC_OUTPUT_DIR) 


clean:
	-rm camera-fb
	-rm $(OBC_OUTPUT_DIR)/camera-fb
























