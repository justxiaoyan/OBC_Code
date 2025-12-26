
CC := gcc


CURDIR := $(realpath .)

.PHONY: all clean autocgroup
all : autocgroup	

autocgroup : autocgroup.c
	@ $(CC) autocgroup.c -o $@ -lpthread
	
clean :
	-rm autocgroup


