SECTIONS
{
    . = 0x80000;
    .text : {
        KEEP(*(.text.boot))  
        *(.text)
        *(.text.*)
    }
    . = ALIGN(16); 
    .rodata : { 
        *(.rodata) 
        *(.rodata.*)
    }
    . = ALIGN(16);
    .data : { 
        *(.data) 
        *(.data.*)
        *(.got)
        *(.got.*)
    }
    . = ALIGN(16);
    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    }
    . = ALIGN(16);
    _end = .;
    
    /* Stack 放在 BSS 之後，向下增長 */
    . = _end + 0x10000;  /* 預留 64KB stack 空間 */
    _stack_top = .;
    
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}
