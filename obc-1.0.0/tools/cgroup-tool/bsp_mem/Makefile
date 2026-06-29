

CC := gcc




CURDIR := $(realpath .)

.PHONY: all clean bsp_mem
all : bsp_mem	

bsp_mem : bsp_mem.c
	@ $(CC) bsp_mem.c -o $@ -lpthread
	
clean :
	-rm bsp_mem


