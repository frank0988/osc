#include "uart.h"
#include "gpio.h"
#include "stddef.h"
#include <stdarg.h>
void                  delay(unsigned int x) {
    while (x--) {
        asm volatile("nop");
    }
}
unsigned int atoi(char *s) {
    unsigned int res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

static uart_fifo_t rx_fifo = { .head = 0, .tail = 0 };
static uart_fifo_t tx_fifo = { .head = 0, .tail = 0 };
static inline uint32_t fifo_count(uart_fifo_t *f) {
    return f->tail - f->head; 
}
static inline int fifo_is_full(uart_fifo_t *f) {
    return fifo_count(f) >= UART_FIFO_SIZE;
}

static inline void fifo_push(char c, uart_fifo_t *f) {
    if (!fifo_is_full(f)) {
        f->buffer[f->tail & FIFO_MASK] = c;
        f->tail++; 
    }
}

static inline char fifo_pop(uart_fifo_t *f) {
    char c = f->buffer[f->head & FIFO_MASK];
    f->head++; 
    return c;
}
void uart_init() {
    volatile unsigned int reg;
    reg = *GPFSEL1;
    // [NOTE] GPFSEL1 manages GPIO 10-19.
    // Each pin takes 3 bits. GPIO 14 is index 4, so shift = 4 * 3 = 12.
    // 7 (0b111) is the mask to clear these 3 bits.
    // 2(0b010) is alt5 mode;
    reg &= ~((7 << 12) | (7 << 15));
    reg |= ((2 << 12) | (2 << 15));
    *GPFSEL1 = reg;
    *GPPUD   = 0;
    delay(150);
    *GPPUDCLK0 = ((1 << 14) | (1 << 15));  // pin 14 pin 15
    delay(150);
    *GPPUDCLK0 = 0;

    *AUX_ENABLES     = 1;
    *AUX_MU_CNTL_REG = 0;
    *AUX_MU_IER_REG  = 0;
    *AUX_MU_LCR_REG  = 3;
    *AUX_MU_MCR_REG  = 0;
    *AUX_MU_BAUD_REG = 270;
    *AUX_MU_IIR_REG  = 6;
    *AUX_MU_CNTL_REG = 3;

    *AUX_MU_IER_REG = 1; 
    *ENABLE_IRQs1 = 1<<29; // Enable AUX interrupt in the interrupt controller
}
void rx_fifo_push(char c) {
    fifo_push(c, &rx_fifo);
}

char tx_fifo_pop(void) {
    return fifo_pop(&tx_fifo);
}

uint32_t tx_fifo_count(void) {
    return fifo_count(&tx_fifo);
}

int tx_fifo_is_empty(void) {
    return fifo_count(&tx_fifo) == 0;
}
void uart_send(unsigned int c) {
    /*
    while (!(*AUX_MU_LSR_REG & 0x20)) {
        
        //bit_5 == 1 -> writable
        //0x20 = 0000 0000 0010 0000
        //ref BCM2837-ARM-Peripherals p5
        asm volatile("nop");
    }
    // write data to AUX_MU_IO_REG
    *AUX_MU_IO_REG = c;
    */
    while (fifo_count(&tx_fifo) >= UART_FIFO_SIZE) {
        // 等待 FIFO 有空位
        asm volatile("nop");
    }
    fifo_push((char)c, &tx_fifo);
    *AUX_MU_IER_REG |= 0x02;
}
char uart_getc() {
    /*
    while (!(*AUX_MU_LSR_REG & 0x01)) {
        asm volatile("nop");
    }
    return (char)(*AUX_MU_IO_REG);
    */
    
    // 只要 FIFO 是空的就等待 (資料由 ISR 負責塞入)
    while (fifo_count(&rx_fifo) == 0) {
        asm volatile("nop");
    }
    return fifo_pop(&rx_fifo);

}

void uart_readline_for_boot(char *buf) {
    int  i = 0;
    char c;
    while (1) {
        c = uart_getc();
        if (c == '\r') {
            c = uart_getc();  // consume the \n after \r
            if (i == 0) continue;
            buf[i] = '\0';
            break;
        }
        if (c == '\n') {
            if (i == 0) continue;
            buf[i] = '\0';
            break;
        }
        buf[i++] = c;
    }
}
void uart_readline(char *buf, int max_len) {
    int idx = 0;
    while (1) {
        char c = uart_getc();
        // end at \r \n
        if (c == '\r' || c == '\n') {
            buf[idx] = '\0';
            uart_puts("\r\n");
            break;
        }
        // bakespace
        if (c == 8 || c == 127) {
            if (idx > 0) {
                idx--;
                uart_puts("\b \b");
            }
        } else {
            if (idx < max_len - 1) {
                buf[idx++] = c;
                uart_send(c);
            }
        }
    }
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_send('\r');
        uart_send(*s++);
    }
}
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
void uart_hex(unsigned int d) {
    unsigned int n;
    int          c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}
// copy from anther way ,need to understand
char *strchr(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s++) return NULL;
    }
    return (char *)s;
}
char *strtok(char *str, const char *delim) {
    static char *next_ptr = NULL;
    if (str != NULL) {
        next_ptr = str;
    }

    if (next_ptr == NULL || *next_ptr == '\0') {
        return NULL;
    }

    char *start = next_ptr;
    while (*start != '\0' && strchr(delim, *start)) {
        start++;
    }

    if (*start == '\0') {
        next_ptr = NULL;
        return NULL;
    }

    char *end = start;
    while (*end != '\0' && !strchr(delim, *end)) {
        end++;
    }

    if (*end != '\0') {
        *end     = '\0';
        next_ptr = end + 1;
    } else {
        next_ptr = NULL;
    }

    return start;
}
void uart_put_hex(uint64_t v) {
    char hex_table[] = "0123456789ABCDEF";
    for (int i = 60; i >= 0; i -= 4) {
        uart_send(hex_table[(v >> i) & 0xF]);
    }
}void uart_put_hex_32(uint32_t v) {
    char hex_table[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        uart_send(hex_table[(v >> i) & 0xF]);
    }
}





void uart_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            if (*p == '\n') uart_send('\r');
            uart_send(*p);
            continue;
        }

        p++; // 跳過 '%'
        
        // 檢查長度修飾符 'l'
        int is_long = 0;
        if (*p == 'l') {
            is_long = 1;
            p++;
        }
        
        switch (*p) {
            case 'c':
                uart_send((char)va_arg(args, int));
                break;
            case 's':
                uart_puts(va_arg(args, char *));
                break;
            case 'd': { // 有號整數 (帶溢位防護)
                if (is_long) {
                    long long v = va_arg(args, long long);
                    if (v < 0) {
                        uart_send('-');
                        // 使用無號型別避免 INT_MIN 溢位
                        unsigned long long abs_v = (unsigned long long)(-v);
                        char buf[24]; int i = 0;
                        if (abs_v == 0) buf[i++] = '0';
                        while (abs_v > 0) { buf[i++] = (abs_v % 10) + '0'; abs_v /= 10; }
                        while (i > 0) uart_send(buf[--i]);
                    } else {
                        char buf[24]; int i = 0;
                        unsigned long long abs_v = (unsigned long long)v;
                        if (abs_v == 0) buf[i++] = '0';
                        while (abs_v > 0) { buf[i++] = (abs_v % 10) + '0'; abs_v /= 10; }
                        while (i > 0) uart_send(buf[--i]);
                    }
                } else {
                    int v = va_arg(args, int);
                    if (v < 0) {
                        uart_send('-');
                        // 使用無號型別避免 INT_MIN 溢位
                        unsigned int abs_v = (unsigned int)(-v);
                        char buf[12]; int i = 0;
                        if (abs_v == 0) buf[i++] = '0';
                        while (abs_v > 0) {
                            buf[i++] = (abs_v % 10) + '0';
                            abs_v /= 10;
                        }
                        while (i > 0) uart_send(buf[--i]);
                    } else {
                        char buf[12]; int i = 0;
                        unsigned int abs_v = (unsigned int)v;
                        if (abs_v == 0) buf[i++] = '0';
                        while (abs_v > 0) {
                            buf[i++] = (abs_v % 10) + '0';
                            abs_v /= 10;
                        }
                        while (i > 0) uart_send(buf[--i]);
                    }
                }
                break;
            }
            case 'u': { // 無號整數
                if (is_long) {
                    unsigned long long v = va_arg(args, unsigned long long);
                    char buf[24]; int i = 0;
                    if (v == 0) buf[i++] = '0';
                    while (v > 0) { buf[i++] = (v % 10) + '0'; v /= 10; }
                    while (i > 0) uart_send(buf[--i]);
                } else {
                    unsigned int v = va_arg(args, unsigned int);
                    char buf[12];
                    int i = 0;
                    if (v == 0) buf[i++] = '0';
                    while (v > 0) {
                        buf[i++] = (v % 10) + '0';
                        v /= 10;
                    }
                    while (i > 0) uart_send(buf[--i]);
                }
                break;
            }
            case 'x': { // 十六進制小寫 (固定寬度: 32位=8字, 64位=16字)
                if (is_long) {
                    uint64_t v = va_arg(args, uint64_t);
                    for (int i = 60; i >= 0; i -= 4) {
                        uart_send("0123456789abcdef"[(v >> i) & 0xF]);
                    }
                } else {
                    uint32_t v = va_arg(args, uint32_t);
                    for (int i = 28; i >= 0; i -= 4) {
                        uart_send("0123456789abcdef"[(v >> i) & 0xF]);
                    }
                }
                break;
            }
            case 'X': { // 十六進制大寫 (固定寬度: 32位=8字, 64位=16字)
                if (is_long) {
                    uint64_t v = va_arg(args, uint64_t);
                    for (int i = 60; i >= 0; i -= 4) {
                        uart_send("0123456789ABCDEF"[(v >> i) & 0xF]);
                    }
                } else {
                    uint32_t v = va_arg(args, uint32_t);
                    for (int i = 28; i >= 0; i -= 4) {
                        uart_send("0123456789ABCDEF"[(v >> i) & 0xF]);
                    }
                }
                break;
            }
            case 'p': { // 指標 (64-bit 16進制，帶 0x 前綴)
                uint64_t v = va_arg(args, uint64_t);
                uart_puts("0x");
                for (int i = 60; i >= 0; i -= 4) {
                    uart_send("0123456789abcdef"[(v >> i) & 0xF]);
                }
                break;
            }
            case '%':
                uart_send('%');
                break;
            default:
                uart_send(*p);
                break;
        }
    }
    va_end(args);
}