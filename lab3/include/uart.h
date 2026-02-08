#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>
#define MMIO_BASE 0x3F000000
#define AUX_BASE  MMIO_BASE + 0x215000
#define IRQ_BASE  MMIO_BASE + 0xB000

#define AUX_ENABLES     ((volatile unsigned int *)(AUX_BASE + 0x04))
#define AUX_MU_CNTL_REG ((volatile unsigned int *)(AUX_BASE + 0x60))
#define AUX_MU_IER_REG  ((volatile unsigned int *)(AUX_BASE + 0x44))
#define AUX_MU_LCR_REG  ((volatile unsigned int *)(AUX_BASE + 0x4C))
#define AUX_MU_LSR_REG  ((volatile unsigned int *)(AUX_BASE + 0x54))
#define AUX_MU_MCR_REG  ((volatile unsigned int *)(AUX_BASE + 0x50))
#define AUX_MU_BAUD_REG ((volatile unsigned int *)(AUX_BASE + 0x68))
#define AUX_MU_IIR_REG  ((volatile unsigned int *)(AUX_BASE + 0x48))
#define AUX_MU_IO_REG   ((volatile unsigned int *)(AUX_BASE + 0x40))

#define ENABLE_IRQs1    ((volatile unsigned int *)(IRQ_BASE + 0x210))

#define UART_FIFO_SIZE 256
#define FIFO_MASK (UART_FIFO_SIZE - 1)
typedef struct{
    char buffer[UART_FIFO_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} uart_fifo_t;

unsigned int atoi(char *s);

void rx_fifo_push(char c);
char tx_fifo_pop(void);
uint32_t tx_fifo_count(void);
int tx_fifo_is_empty(void);

void  uart_init();
void  uart_send(unsigned int c);
void  uart_readline(char *buf, int max_len);
void  uart_readline_for_boot(char *buf); 
char  uart_getc();
void  uart_puts(const char *s);
int   strcmp(const char *s1, const char *s2);
int   strncmp(const char *s1, const char *s2, size_t n);
void  uart_hex(unsigned int d);
char *strtok(char *str, const char *delim);
void uart_put_hex(uint64_t v);
void uart_put_hex_32(uint32_t v);
void uart_printf(const char *fmt, ...);
#endif
