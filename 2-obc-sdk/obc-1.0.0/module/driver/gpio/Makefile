#demo_makefile


obj-m:=gpio.o
ARCH ?= arm


KBUILD_CFLAGS += -g

EXTRA_CFLAGS  += -g "-DBUILD_TIME=\"$(shell date +'%Y-%m-%d %H:%M:%S')\""

PWD:=$(shell pwd)
default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SDK_DIR) M=$(PWD) modules

clean:
	rm -rf *.mk .tmp_versions Module.symvers *.mod.c *.o *.ko .*.cmd Module.markers modules.order *.a *.mod