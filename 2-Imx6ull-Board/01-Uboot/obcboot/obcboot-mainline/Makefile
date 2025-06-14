
OBCBOOT_LDSCRIPT := $(OBCBOOT_SRC_DIR)/board/imx/imx6ull/obcboot.lds


OBCBOOTBIN := obcboot
OBCBOOT_COBJS-y += common/obcboot.o
OBCBOOT_OBJS-y += common/main.o
OBCBOOT_OBJS-y += common/obcbase.o

# 搴撲緷璧�
OBCBOOT_LIBOBJS-y += common/stubs.o
OBCBOOT_LIB-y += $(OBCBOOT_SRC_DIR)/common/libstubs.o


# 澶存枃浠惰矾寰�
OBCBOOT_INCLUDE-y += -I$(OBCBOOT_SRC_DIR)/common
OBCBOOT_INCLUDE-y += -I$(OBCBOOT_SRC_DIR)/include
#OBCBOOT_INCLUDE-y += -I$(OBCBOOT_SRC_DIR)/util-platform


# 閫夋嫨瀵瑰簲鐗堟湰鐨勭殑makefile
include $(OBCBOOT_SRC_DIR)/scripts/Makefile


.PHONY: clean

# 娓呴櫎涓�闂村唴瀹�
clean:
	@find $(OBCBOOT_SRC_DIR) \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '*.ko.*' -o -name '*.su' -o -name '*.cfgtmp' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name modules.builtin -o -name '.tmp_*.o.*' \
		-o -name 'dsdt.aml' -o -name 'dsdt.asl.tmp' -o -name 'dsdt.c' \
		-o -name '*.gcno' \) -type f -print | xargs rm -f
	@rm -f $(OBCBOOT_SRC_DIR)/$(OBCBOOTBIN) \
			$(OBCBOOT_SRC_DIR)/$(OBCBOOTBIN).bin \
			$(OBCBOOT_SRC_DIR)/$(OBCBOOTBIN).srec \
			$(OBCBOOT_SRC_DIR)/System.map
	@rm -rf $(OBCBOOT_SRC_DIR)/data1
	