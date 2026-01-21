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
    adr x0, exception_vector_table
    msr vbar_el1, x0
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
from_el1_to_el0:
    // x0: user program 的起始地址
    // x1: user program 的 stack pointer (SP_EL0)
    
    msr elr_el1, x0      // 設定回到 EL0 後要執行的地址
    msr sp_el0, x1       // 設定 EL0 的 Stack Pointer
    
    // 設定 SPSR_EL1 使得 eret 後進入 EL0
    // 0x3c0 代表:
    // - [9:6] DAIF 遮罩 (1111) 暫時關閉中斷
    // - [4]   Mode 0: AArch64
    // - [3:0] M[3:0] 0000: EL0t
    mov x2, 0x3c0
    msr spsr_el1, x2
    
    eretmov x0, 0 // EL0 uses aarch64
    msr spsr_el1, 0x3c0
    msr elr_el1, lr
    eret // return to EL0

.global dtb_ptr
.section .data
dtb_ptr: .dc.a 0x0