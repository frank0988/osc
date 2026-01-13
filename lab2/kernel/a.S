.section ".text.boot"
.global _start

_start:
    // Check processor ID is zero (Core 0), else hang.
    mrs     x0, mpidr_el1
    and     x0, x0, #3
    cbz     x0, master
    b       hang

master:
    // Initialize stack pointer.
    ldr     x0, =_stack_top
    mov     sp, x0

    // Clear BSS section.
    ldr     x0, =_bss_start
    ldr     x1, =_bss_end
    
    // Check if BSS size is 0, skip if true.
    cmp     x0, x1
    b.ge    enter_main

bss_loop:
    str     xzr, [x0], #8
    cmp     x0, x1
    b.lt    bss_loop

enter_main:
    // Jump to C main function.
    bl      main

hang:
    // Infinite loop for other cores or if main returns.
    wfe
    b       hang

.global put32
put32:
    str w1, [x0]   
    ret

// unsigned int get32(long addr)
.global get32
get32:
    ldr w0, [x0]    
    ret
