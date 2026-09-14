__STACK_SIZE = 0x20000;
__TI_STACK_SIZE = __STACK_SIZE;


ENTRY(Entry)

MEMORY
{
    DDR_MPU1   (RWX)   : ORIGIN = 0x80000000, LENGTH = 0x08000000

}


/* 关键修改：将启动代码映射到EL3区域 */
REGION_ALIAS("REGION_TEXT_STARTUP", DDR_MPU1);
REGION_ALIAS("REGION_TEXT_EL3", DDR_MPU1);
REGION_ALIAS("REGION_TEXT", DDR_MPU1);
REGION_ALIAS("REGION_BSS", DDR_MPU1);
REGION_ALIAS("REGION_DATA", DDR_MPU1);
REGION_ALIAS("REGION_STACK", DDR_MPU1);
REGION_ALIAS("REGION_HEAP", DDR_MPU1);
REGION_ALIAS("REGION_ARM_EXIDX", DDR_MPU1);
REGION_ALIAS("REGION_ARM_EXTAB", DDR_MPU1);
REGION_ALIAS("REGION_DATA_BUFFER", DDR_MPU1);
REGION_ALIAS("IPC_DATA_BUFFER_1", DDR_MPU1);

SECTIONS {

    .text.el3 : {
        *(.text.el3)
        . = ALIGN(8);
        __RT_SVC_DESCS_START__ = .;
        KEEP(*(rt_svc_descs))
        __RT_SVC_DESCS_END__ = .;
    } > REGION_TEXT_EL3 AT> REGION_TEXT_EL3

    .text.csl_a55_startup : {
        *(.head.text)
        *(.text.csl_a55_startup)
        //*(.Entry)
    } > REGION_TEXT_STARTUP AT> REGION_TEXT_STARTUP

    .text : {
        CREATE_OBJECT_SYMBOLS
        *(.text)
        *(.text.*)
        . = ALIGN(0x8);
        KEEP (*(.ctors))
        . = ALIGN(0x4);
        KEEP (*(.dtors))
        . = ALIGN(0x8);
        __init_array_start = .;
        KEEP (*(.init_array*))
        __init_array_end = .;
        *(.init)
        *(.fini*)
    } > REGION_TEXT AT> REGION_TEXT

    PROVIDE (__etext = .);
    PROVIDE (_etext = .);
    PROVIDE (etext = .);

    .rodata : {
        *(.rodata)
        *(.rodata*)
    } > REGION_TEXT AT> REGION_TEXT

    .data_buffer : ALIGN (8) {
        __data_buffer_load__ = LOADADDR (.data_buffer);
        __data_buffer_start__ = .;
        *(.data_buffer)
        *(.data_buffer*)
        . = ALIGN (8);
        __data_buffer_end__ = .;
    } > REGION_DATA_BUFFER AT> REGION_DATA_BUFFER

    .data : ALIGN (8) {
        __data_load__ = LOADADDR (.data);
        __data_start__ = .;
        *(.data)
        *(.data*)
        . = ALIGN (8);
        __data_end__ = .;
    } > REGION_DATA AT> REGION_TEXT

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > REGION_ARM_EXIDX AT> REGION_ARM_EXIDX

    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > REGION_ARM_EXTAB AT> REGION_ARM_EXTAB

    .bss:extMemCache:ramdisk : {
    } > DDR_MPU1  /* MSMC_MPU1 */

    /* USB or any other LLD buffer for benchmarking */
    .benchmark_buffer (NOLOAD) : ALIGN (32) {
    } > DDR_MPU1

    .bss:frameBuffer (NOLOAD) : ALIGN (32) {
    } > DDR_MPU1

    ipc_data_buffer (NOLOAD) : ALIGN (32) {
    } > IPC_DATA_BUFFER_1

    .bss : {
        __bss_start__ = .;
        *(.shbss)
        *(.bss)
        *(.bss.*)
        . = ALIGN (8);
        __bss_end__ = .;
        . = ALIGN (8);
        *(COMMON)
    } > REGION_BSS AT> REGION_BSS

    .heap : {
        __heap_start__ = .;
        end = __heap_start__;
        _end = end;
        __end = end;
        KEEP(*(.heap))
        __heap_end__ = .;
        __HeapLimit = __heap_end__;
    } > REGION_HEAP AT> REGION_HEAP

    .stack (NOLOAD) : ALIGN(16) {
        _stack = .;
        __stack = .;
        KEEP(*(.stack))
    } > REGION_STACK AT> REGION_STACK

	__STACK_BASE = __stack;
}
