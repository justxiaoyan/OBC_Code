# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.


#extra-y        := hello_world
extra-y        := $(OBCBOOTBIN)

#
# Some versions of make do not handle trailing white spaces properly;
# leading to build failures. The problem was found with GNU Make 3.80.
# Using 'strip' as a workaround for the problem.
#
ELF := $(strip $(extra-y))

extra-y += $(addsuffix .srec,$(extra-y)) $(addsuffix .bin,$(extra-y))
clean-files  := *.srec *.bin

COBJS-y	+= $(OBCBOOT_COBJS-y)
COBJS	:= $(addprefix $(obj)/,$(COBJS-y))
#COBJS	:= $(ELF:=.o)

UBOOTINCLUDE += $(OBCBOOT_INCLUDE-y)

#LIB	= $(obj)/libstubs.o
LIB	+= $(OBCBOOT_LIB-y)

LIBOBJS-$(CONFIG_PPC) += ppc_longjmp.o ppc_setjmp.o
#LIBOBJS-y += stubs.o
LIBOBJS-y += $(OBCBOOT_LIBOBJS-y) $(OBCBOOT_OBJS-y)

.SECONDARY: $(call objectify,$(COBJS))
targets += $(patsubst $(obj)/%,%,$(LIB)) $(COBJS) $(LIBOBJS-y)
targets += $(ALL-y)


LIBOBJS	:= $(addprefix $(obj)/,$(LIBOBJS-y))
ELF	:= $(addprefix $(obj)/,$(ELF))

# For PowerPC there's no need to compile standalone applications as a
# relocatable executable.  The relocation data is not needed, and
# also causes the entry point of the standalone application to be
# inconsistent.
ifeq ($(CONFIG_PPC),y)
PLATFORM_CPPFLAGS := $(filter-out $(RELFLAGS),$(PLATFORM_CPPFLAGS))
endif


# We don't want gcc reordering functions if possible.  This ensures that an
# application's entry point will be the first function in the application's
# source file.
ccflags-y += $(call cc-option,-fno-toplevel-reorder)

#########################################################################

quiet_cmd_link_lib = LD      $@
      cmd_link_lib = $(LD) $(ld_flags) -r -o $@ $(filter $(LIBOBJS), $^)

$(LIB):	$(LIBOBJS) FORCE
	$(call if_changed,link_lib)

CONFIG_OBCBOOT_LOAD_ADDR := 0x82000000
#cmd_link_elf = $(LD) $(LDFLAGS) -g -T $(OBCBOOT_LDSCRIPT) -Ttext $(CONFIG_OBCBOOT_LOAD_ADDR)
#cmd_link_elf = $(LD) $(LDFLAGS) -g -Ttext $(CONFIG_OBCBOOT_LOAD_ADDR)
quiet_cmd_link_elf = LD      $@
		cmd_link_elf = $(LD) $(LDFLAGS) -g -T $(OBCBOOT_LDSCRIPT) -Ttext $(CONFIG_OBCBOOT_LOAD_ADDR) \
				-o $@ $< $(LIB) $(OBJS) $(PLATFORM_LIBGCC)

$(ELF): $(obj)/%: $(COBJS) $(LIB) FORCE
	$(call if_changed,link_elf)

$(obj)/%.srec: OBJCOPYFLAGS := -O srec
$(obj)/%.srec: $(obj)/% FORCE
	$(call if_changed,objcopy)

$(obj)/%.bin: OBJCOPYFLAGS := -O binary
$(obj)/%.bin: $(obj)/% $(ALL-y) FORCE
	$(call if_changed,objcopy)

# some files can only build in ARM or THUMB2, not THUMB1

ifdef CONFIG_SYS_THUMB_BUILD
ifndef CONFIG_HAS_THUMB2

CFLAGS_stubs.o := -marm

endif
endif

KBUILD_CFLAGS += -DUBOOTVERSION=$(VERSION)$(PATCHLEVEL)

