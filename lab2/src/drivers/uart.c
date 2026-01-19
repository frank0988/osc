#include "uart.h"
#include "gpio.h"
typedef unsigned long size_t;
void                  delay(unsigned int x) {
    while (x--) {
        asm volatile("nop");
    }
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
}
void uart_send(unsigned int c) {
    while (!(*AUX_MU_LSR_REG & 0x20)) {
        /*
        bit_5 == 1 -> writable
        0x20 = 0000 0000 0010 0000
        ref BCM2837-ARM-Peripherals p5
        */
        asm volatile("nop");
    }
    // write data to AUX_MU_IO_REG
    *AUX_MU_IO_REG = c;
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
char uart_getc() {
    while (!(*AUX_MU_LSR_REG & 0x01)) {
        asm volatile("nop");
    }
    return (char)(*AUX_MU_IO_REG);
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