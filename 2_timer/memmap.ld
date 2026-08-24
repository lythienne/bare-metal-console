SECTIONS
{
    .text 0x40000000      : { *(.text.start) *(.text*) }
    __text_end            = .;
    .rodata               : { *(.rodata*) *(.srodata*) }
    .data  ALIGN(0x20000) : { *(.sdata*) }
    __bss_start           = .;
    .bss                  : { *(.bss*)  *(.sbss*) *(COMMON) }
    __bss_end             = .;
}

/* Force link of _start and verify correct position */
ENTRY(_start)
ASSERT(_start == ADDR(.text), "_start symbol must be placed first in text section")
