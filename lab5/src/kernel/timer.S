.section ".text"
.global core_timer_enable
.global core_timer_handler
.global get_cntfrq_el0
.global get_cntpct_el0

get_cntfrq_el0:
    mrs x0, cntfrq_el0
    ret

get_cntpct_el0:
    mrs x0, cntpct_el0
    ret

core_timer_enable:
    // Enable the core timer (set cntp_ctl_el0 = 1)
    mov x0, 1
    msr cntp_ctl_el0, x0
    
    // Set the timer expired time = cntfrq_el0 >> 5
    mrs x0, cntfrq_el0
    lsr x0, x0, #5
    msr cntp_tval_el0, x0
    
    // Unmask timer interrupt from the first level interrupt controller
    mov x0, 2
    ldr x1, =0x40000040 //CORE0_TIMER_IRQ_CTRL
    str w0, [x1]
    
    ret

core_timer_handler:
    // Set the next timer expired time = cntfrq_el0 >> 5
    mrs x0, cntfrq_el0
    lsr x0, x0, #5
    msr cntp_tval_el0, x0
    
    ret
