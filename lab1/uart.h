#ifndef UART_H
#define UART_H
#define MMIO_BASE 0x3F000000
#define AUX_BASE MMIO_BASE + 0x215000

#define AUX_ENABLES ((volatile unsigned int*)(AUX_BASE + 0x04))
#define AUX_MU_CNTL_REG ((volatile unsigned int*)(AUX_BASE + 0x60))
#define AUX_MU_IER_REG ((volatile unsigned int*)(AUX_BASE + 0x44))
#define AUX_MU_LCR_REG ((volatile unsigned int*)(AUX_BASE + 0x4C))
#define AUX_MU_LSR_REG ((volatile unsigned int*)(AUX_BASE + 0x54))
#define AUX_MU_MCR_REG ((volatile unsigned int*)(AUX_BASE + 0x50))
#define AUX_MU_BAUD_REG ((volatile unsigned int*)(AUX_BASE + 0x68))
#define AUX_MU_IIR_REG ((volatile unsigned int*)(AUX_BASE + 0x48))
#define AUX_MU_IO_REG ((volatile unsigned int*)(AUX_BASE + 0x40))
void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char* s);
int strcmp(const char* s1, const char* s2);
void uart_hex(unsigned int d);
#endif
