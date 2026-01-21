#include "uart.h"

void sync_handler_c() {
    unsigned long spsr, elr, esr;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr));
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    uart_puts("Synchronous Exception Occurred!\n");
    uart_printf("SPSR_EL1: 0x%p\n", spsr);
    uart_printf("ELR_EL1:  0x%p\n", elr);
    uart_printf("ESR_EL1:  0x%p\n", esr);


}
void irq_handler_c() {
    uart_puts("IRQ Exception Occurred!\n");
}