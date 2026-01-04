SECTIONS {
    . = 0x60000;
    _stack_top = .;
    _stext = .;

    .text : {
        KEEP(*(.text.relo))
        *(.text.boot)
        *(.text)
        *(.rodata)
    }

    . = ALIGN(0x1000);
    _etext = .;

    _sdata = .;
    .data : { *(.data) }
    . = ALIGN(0x1000);
    _edata = .; 

    _sbss = .;
    .bss : { 
        *(.bss*) 
        *(COMMON)
    } 
    . = ALIGN(0x1000);
    _ebss = .;  

    _blsize = _ebss - _stext;
}
