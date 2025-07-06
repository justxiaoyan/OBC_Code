
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
    . = 0x82000000;  /* 明确指定代码段的起始地址 */
	.text :
	{
        _start = .;
		__text_start = .;
        __init_end = .;
		*(.text)
	}

	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }

	. = ALIGN(4);
	.data : { *(.data) }

	. = ALIGN(4);
	.got : { *(.got) }

    . = ALIGN(4);
    __bss_start = .;
    .bss : { *(.bss*) }
    _end = .;  /* 显式定义 _end 符号 */
}














