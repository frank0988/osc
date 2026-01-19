.section ".text.relo"
.global _start
#reload bootloader from 0x80000 to 0x60000
_start:
    mov     x21, x0//store dtb to x21


    adr x10, .          //raspiberry pi start
    ldr x11,= _stext      //destination
    cmp x10, x11        //without bootloader
    b.eq end_relo
    
    ldr x12, =_stext
    ldr x13, =_ebss
    sub x13,x13,x12
    add x12,x10,x13

copy_loop:
    ldp x13, x14, [x10], #16 //copy from source address [x10]
    stp x13, x14, [x11], #16 //copy to   target address [x11]
    cmp x10, x12
    b.lo copy_loop

    ldr x16, =end_relo
    br  x16

end_relo:
    ldr x14, =_bl_entry    //jump to boot part 
    br x14

.section ".text.boot"
_bl_entry:
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
    ldr     x0, =dtb_ptr
    str     x21, [x0]

    mov     x0, x21        
    bl      main

hang:
    // Infinite loop for other cores or if main returns.
    wfe
    b       hang
