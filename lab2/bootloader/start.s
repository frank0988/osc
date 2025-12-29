.section ".text.relo"
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
    ldr     x0, =_sbss
    ldr     x1, =_ebss
    
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
