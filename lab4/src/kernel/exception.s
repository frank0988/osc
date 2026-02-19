.macro save_all
    sub sp, sp, 32 * 8
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
.endm


.macro load_all
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm

.macro ventry label
    .align 7
    b \label
.endm

.align 11
.global exception_vector_table
exception_vector_table:
    /* Group 1: Current EL with SP0 */
    ventry sync_handler_el1t
    ventry irq_handler_el1t
    ventry fiq_handler_el1t
    ventry serror_handler_el1t

    /* Group 2: Current EL with SPx */
    ventry sync_handler_el1h
    ventry irq_handler_el1h
    ventry fiq_handler_el1h
    ventry serror_handler_el1h

    /* Group 3: Lower EL (AArch64) */
    ventry sync_handler_el0_64
    ventry irq_handler_el0_64
    ventry fiq_handler_el0_64
    ventry serror_handler_el0_64

    /* Group 4: Lower EL (AArch32) */
    ventry sync_handler_el0_32
    ventry irq_handler_el0_32
    ventry fiq_handler_el0_32
    ventry serror_handler_el0_32

sync_handler_el1h:
    save_all
    bl sync_handler_el1h_c
    load_all
    eret

irq_handler_el1h:
    save_all
    bl irq_handler_el1h_c
    load_all
    eret

/* Exception handlers */

/* Current EL with SP0 - 不应该发生 */
sync_handler_el1t:
    b sync_handler_el1t
irq_handler_el1t:
    b irq_handler_el1t
fiq_handler_el1t:
    b fiq_handler_el1t
serror_handler_el1t:
    b serror_handler_el1t

/* Current EL with SPx - 先挂起FIQ和SError */
fiq_handler_el1h:
    b fiq_handler_el1h
serror_handler_el1h:
    b serror_handler_el1h

/* Lower EL (AArch64) - 处理用户程序的异常 */
sync_handler_el0_64:
    save_all
    bl sync_handler_el0_64_c
    load_all
    eret

irq_handler_el0_64:
    save_all
    bl irq_handler_el0_64_c
    load_all
    eret

fiq_handler_el0_64:
    b fiq_handler_el0_64
serror_handler_el0_64:
    b serror_handler_el0_64

/* Lower EL (AArch32) - 先挂起 */
sync_handler_el0_32:
    b sync_handler_el0_32
irq_handler_el0_32:
    b irq_handler_el0_32
fiq_handler_el0_32:
    b fiq_handler_el0_32
serror_handler_el0_32:
    b serror_handler_el0_32
