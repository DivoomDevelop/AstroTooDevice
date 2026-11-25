OUTPUT_ARCH("riscv")
OUTPUT_FORMAT("elf32-littleriscv","elf32-littleriscv","elf32-littleriscv")
MEMORY
{
    RAM (rwx) : ORIGIN = 0x40010000, LENGTH = 0xff0000
}
__RAM_BASE = ORIGIN(RAM);
__MSP_STACK_LENGTH = 0x100;
ENTRY(_start)
SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        __text_start__ = .;
        KEEP(*(.start))
        *(.vectors)
        *(*.text)
        *(.text*)
        *(.nonxip_text*)
        *(.sram_text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)
        *(.rodata*)
        *(.srodata*)
        *(.nonxip_rodata*)
        *(.sram_rodata*)
        KEEP(*(.eh_frame*))
 . = ALIGN(4096);
        . = ALIGN(0x10000);
        __text_end__ = .;
    } > RAM
    __etext = .;
    _sidata = .;
    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        _sdata = .;
        *(vtable)
        *(.data*)
        *(.sdata*)
        *(.nonxip_data*)
        *(.sram_data*)
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        PROVIDE(__ctors_start__ = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__ctors_end__ = .);
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);
        KEEP(*(.jcr*))
        . = ALIGN(4);
        __usbh_class_info_start__ = .;
        KEEP(*(SORT(.usbh_class_info)));
        __usbh_class_info_end__ = .;
        . = ALIGN(16);
        __init_process_stack_start__ = .;
        . += 4096;
        __init_process_stack_end__ = .;
        . = ALIGN(16);
        __freertos_irq_stack_bottom = .;
        . += 4096;
        __freertos_irq_stack_top = .;
        PROVIDE( __global_pointer$ = . + 0x400 );
        . = ALIGN(4);
        __data_end__ = .;
        _edata = .;
    } > RAM
    .note.gnu.build-id : ALIGN(4) {
        __crash_build_id_start = .;
        KEEP(*(.note.gnu.build-id))
        __crash_build_id_end = .;
    } > RAM
    ASSERT(__crash_build_id_end - __crash_build_id_start == 32, "Crash log requires GNU uuid Build ID")
    .version_table : {
 KEEP(*(.version_table))
   } > RAM
    .resource_table : {
 KEEP(*(.resource_table))
   } > RAM
    .digest : ALIGN(4)
  {
    _digest_start = ABSOLUTE(.);
    KEEP (*(.digest))
    . = ALIGN (4);
    _digest_end = ABSOLUTE(.);
  } > RAM
   .FSymTab : {
        _syscall_table_begin = .;
 KEEP(*(FSymTab))
 _syscall_table_end = .;
   } > RAM
   .VSymTab : {
        __vsymtab_start = .;
 KEEP(*(VSymTab))
 __vsymtab_end = .;
   } > RAM
   .ttcall : {
        _tt_begin = .;
 KEEP(*(ttcall))
 _tt_end = .;
   } > RAM
    .heap (COPY):
    {
        __end__ = .;
        __heap_start__ = .;
        _heap_start = .;
        end = __end__;
        *(.heap*)
        __HeapLimit = .;
    } > RAM
    .stack_dummy (COPY):
    {
        *(.stack*)
    } > RAM
    .bss (NOLOAD):
    {
        . = ALIGN(4);
        __bss_start__ = .;
        _sbss = .;
        *(.bss*)
        *(.sbss*)
        *(COMMON)
        *(.nonxip_bss*)
        *(.sram_bss*)
        . = ALIGN(4);
        __bss_end__ = .;
        _ebss = .;
    } > RAM
    . = ALIGN(8);
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    _estack = __StackTop;
    __heap_end__ = _estack - __MSP_STACK_LENGTH;
    _heap_end = __heap_end__;
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
