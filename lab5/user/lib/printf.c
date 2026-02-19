// 簡易 printf 實作，基於 uart_write syscall
#include "syscall.h"

// 內部：將整數轉成十進位字串
static int itoa(long val, char *buf, int base, int is_signed) {
    char tmp[32];
    int i = 0;
    int neg = 0;
    unsigned long uval;

    if (is_signed && val < 0) {
        neg = 1;
        uval = (unsigned long)(-val);
    } else {
        uval = (unsigned long)val;
    }

    if (uval == 0) {
        tmp[i++] = '0';
    } else {
        while (uval > 0) {
            int digit = uval % base;
            tmp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            uval /= base;
        }
    }

    int pos = 0;
    if (neg) buf[pos++] = '-';
    while (i > 0) {
        buf[pos++] = tmp[--i];
    }
    return pos;
}


// variadic argument support using builtins
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char buf[256];
    int pos = 0;

    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] == '%' && fmt[i + 1]) {
            i++;
            char numbuf[32];
            int len;
            switch (fmt[i]) {
                case 'd': {
                    int val = va_arg(ap, int);
                    len = itoa(val, numbuf, 10, 1);
                    for (int j = 0; j < len && pos < 255; j++)
                        buf[pos++] = numbuf[j];
                    break;
                }
                case 'x': {
                    unsigned long val = va_arg(ap, unsigned long);
                    len = itoa(val, numbuf, 16, 0);
                    buf[pos++] = '0';
                    buf[pos++] = 'x';
                    for (int j = 0; j < len && pos < 255; j++)
                        buf[pos++] = numbuf[j];
                    break;
                }
                case 's': {
                    const char *s = va_arg(ap, const char *);
                    if (!s) s = "(null)";
                    while (*s && pos < 255)
                        buf[pos++] = *s++;
                    break;
                }
                case '%':
                    buf[pos++] = '%';
                    break;
                default:
                    buf[pos++] = '%';
                    buf[pos++] = fmt[i];
                    break;
            }
        } else {
            if (pos < 255)
                buf[pos++] = fmt[i];
        }
    }

    va_end(ap);

    // 輸出到 UART
    uart_write(buf, pos);
    return pos;
}
