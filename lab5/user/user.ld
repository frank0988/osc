/* User program linker script */
/* 程式會被載入到記憶體中的任意位置（由 kernel 決定）*/
/* 我們使用 position-independent 的方式，但先用固定基址 */

SECTIONS
{
    . = 0x0;  /* 程式從 offset 0 開始，實際載入地址由 kernel 的 cpio_get_file 決定 */

    .text : {
        *(.text.entry)    /* crt0 _start 放最前面 */
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    
    .data : { 
        *(.data*) 
        *(.got*)
        *(.plt*)
        *(.dynamic)
        *(.rela*)
        *(.rel*)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
}
