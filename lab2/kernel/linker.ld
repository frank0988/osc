SECTIONS
{
    . = 0x80000;
    .text : {
        KEEP(*(.text.boot))  
        *(.text)
    }
    . = ALIGN(16); 
    .rodata : { *(.rodata) }
    . = ALIGN(16);
    .data : { *(.data) }
    . = ALIGN(16);
    .bss : {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;
    }
    . = ALIGN(16);
    _end - .;
    _stack_top = 0x80000;
}
