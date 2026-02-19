#include "uart.h"
#include "timer.h"
#include "exception.h"
#include "thread.h"
#include "syscall.h"

// 輔助函數：獲取當前 EL 級別並格式化輸出
const char* get_current_el_str() {
    unsigned long currentel;
    asm volatile("mrs %0, currentel" : "=r"(currentel));
    
    // CurrentEL 寄存器的 [3:2] bits 表示 EL 級別
    unsigned long el = (currentel >> 2) & 0x3;
    
    switch(el) {
        case 0: return "EL0";
        case 1: return "EL1";
        case 2: return "EL2";
        case 3: return "EL3";
        default: return "Unknown";
    }
}

void sync_handler_el1h_c(struct trap_frame *tf) {
    unsigned long esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    unsigned long ec = (esr >> 26) & 0x3f;  // Exception Class
    
    // 使用 polling 版本，因為在 exception 中 IRQ 被禁用
    uart_printf_polling("[EL1h Sync Exception]\n");
    uart_printf_polling("SPSR_EL1: 0x%p\n", tf->spsr_el1);
    uart_printf_polling("ELR_EL1:  0x%p\n", tf->elr_el1);
    uart_printf_polling("ESR_EL1:  0x%p\n", esr);
    uart_printf_polling("Exception Class: 0x%x\n", ec);
    
    // EC == 0x15: SVC from AArch64
    if (ec == 0x15) {
        // AArch64 SVC 已自動將 ELR 設為下一條指令，不需 += 4
    } else {
        // 其他 exception (如 Data Abort, Undefined Instruction): Panic
        uart_printf_polling("PANIC: Unhandled exception at EL1h, halting.\n");
        while (1) {
            asm volatile("wfe");
        }
    }
}


void handle_mini_uart_irq(){
    uint32_t iir = *AUX_MU_IIR_REG;
    
    // bit 0: 0=interrupt pending, 1=no interrupt
    if (iir & 0x01) {
        return; // 沒有中斷
    }
    // bit 2-1: 中斷類型識別
    // 00 = modem, 01 = TX ready, 10 = RX ready
    uint32_t int_id = (iir >> 1) & 0x03;
    
    if (int_id == 0x02) {
        // RX interrupt: 有數據可讀
        while (*AUX_MU_LSR_REG & 0x01) {
            char c = (char)(*AUX_MU_IO_REG);
            rx_fifo_push(c);
        }
    } 
    else if (int_id == 0x01) {
        // TX interrupt: 發送器空閒
        while ((*AUX_MU_LSR_REG & 0x20) && tx_fifo_count() > 0) {
            char c = tx_fifo_pop();
            *AUX_MU_IO_REG = c;
        }
        
        // 沒有更多數據時禁用 TX interrupt
        if (tx_fifo_count() == 0) {
            *AUX_MU_IER_REG &= ~0x02;
        }
    }
    
}
void irq_handler_el1h_c() {
    unsigned int source = *CORE0_IRQ_SOURCE;

    if (source & (1 << 1)) {
        // 來源是 Core Timer
        //uart_printf_polling("{%d}", current_thread ? current_thread->id : -1);
        handle_core_irq();
        // 只在 scheduler 已啟動後才做搶佔排程
        // （避免在 kernel_main 初始化期間誤觸 schedule）
        if (current_thread) {
            schedule();
        }
    } 
    else if (source & (1 << 8)) {
        // 來源是周邊，進一步檢查
        unsigned int p1 = *IRQ_PENDING_1;

        if (p1 & (1 << 29)) {
            handle_mini_uart_irq();
        } else {
            // 未知的周邊中斷，屏蔽它以避免 livelock
            // 透過 DISABLE_IRQs1 屏蔽未處理的中斷位元
            *DISABLE_IRQs1 = p1 & ~(1 << 29);  // 屏蔽除了 UART 以外的中斷
        }
    }
    // 其他未知的 source 位元會被忽略（它們會自動清除或屬於其他處理流程）
}

void sync_handler_el0_64_c(struct trap_frame *tf) {
    unsigned long esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    unsigned long ec = (esr >> 26) & 0x3f;  // Exception Class
    
    // EC == 0x15: SVC from AArch64
    if (ec == 0x15) {
        // 注意：AArch64 的 SVC 例外已自動將 ELR_EL1 設為 SVC 的下一條指令 (PC+4)
        // 不需要手動 += 4，否則會跳過 syscall wrapper 中的 ret 指令
        syscall_dispatch(tf);
    } else {
        // 其他 exception: 印出除錯資訊後 Panic
        uart_printf_polling("[EL0_64 Sync Exception]\n");
        uart_printf_polling("SPSR_EL1: 0x%p\n", tf->spsr_el1);
        uart_printf_polling("ELR_EL1:  0x%p\n", tf->elr_el1);
        uart_printf_polling("ESR_EL1:  0x%p\n", esr);
        uart_printf_polling("Exception Class: 0x%x\n", ec);
        uart_printf_polling("PANIC: Unhandled exception from EL0, halting.\n");
        while (1) {
            asm volatile("wfe");
        }
    }
}

void irq_handler_el0_64_c(struct trap_frame *tf) {
    (void)tf;
    
    unsigned int source = *CORE0_IRQ_SOURCE;

    if (source & (1 << 1)) {
        // 來源是 Core Timer
        handle_core_irq();
        // EL0 搶佔排程：讓 fork 出的 child 有機會執行
        if (current_thread) {
            schedule();
        }
    } 
    else if (source & (1 << 8)) {
        // 來源是周邊，進一步檢查
        unsigned int p1 = *IRQ_PENDING_1;

        if (p1 & (1 << 29)) {
            handle_mini_uart_irq();
        } else {
            // 未知的周邊中斷，屏蔽它以避免 livelock
            *DISABLE_IRQs1 = p1 & ~(1 << 29);
        }
    }
}