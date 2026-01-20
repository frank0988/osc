.section ".text.boot"
.global _start

_start:
    ldr    x1, =dtb_ptr
    str    x0, [x1]
    mov     x21, x0
    // Check processor ID is zero (Core 0), else hang.
    mrs     x0, mpidr_el1
    and     x0, x0, #3
    cbz     x0, master
    b       hang

master:
    bl from_el2_to_el1
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
    mov     x0, x21
    bl      kernel_main

hang:
    // Infinite loop for other cores or if main returns.
    wfe
    b       hang
from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1

.global dtb_ptr
.section .data
dtb_ptr: .dc.a 0x0