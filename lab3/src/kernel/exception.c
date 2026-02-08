#include "uart.h"
#include "timer.h"
#include "exception.h"

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

void sync_handler_el1h_c() {
    unsigned long spsr, elr, esr;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr));
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    asm volatile("mrs %0, esr_el1" : "=r"(esr));


    uart_printf("SPSR_EL1: 0x%p\n", spsr);
    uart_printf("ELR_EL1:  0x%p\n", elr);
    uart_printf("ESR_EL1:  0x%p\n", esr);


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
        handle_core_irq();
    } 
    else if (source & (1 << 8)) {
        // 來源是周邊，進一步檢查
        unsigned int p1 = *IRQ_PENDING_1;
        //unsigned int p2 = *IRQ_PENDING_2;

        if (p1 & (1 << 29)) {
            handle_mini_uart_irq();
        } 
    }
}

void sync_handler_el0_64_c() {
    unsigned long spsr, elr, esr;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr));
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    asm volatile("mrs %0, esr_el1" : "=r"(esr));


    uart_printf("SPSR_EL1: 0x%p\n", spsr);
    uart_printf("ELR_EL1:  0x%p\n", elr);
    uart_printf("ESR_EL1:  0x%p\n", esr);
    
    unsigned long ec = (esr >> 26) & 0x3f;  // Exception Class
    uart_printf("Exception Class: 0x%x\n", ec);
    
    
}

void irq_handler_el0_64_c() {
    unsigned int source = *CORE0_IRQ_SOURCE;

    if (source & (1 << 1)) {
        // 來源是 Core Timer
        handle_core_irq();
    } 
    else if (source & (1 << 8)) {
        // 來源是周邊，進一步檢查
        unsigned int p1 = *IRQ_PENDING_1;
        //unsigned int p2 = *IRQ_PENDING_2;

        if (p1 & (1 << 29)) {
            handle_mini_uart_irq();
        } 
    }
}