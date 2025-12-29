#include "uart.h"
extern char *dtb_ptr;
unsigned int atoi(char *s) {
    unsigned int res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}
void load_kernel() {
    char *kernel_start = (char *)0x80000;
    char  buf[64];
    uart_readline(buf);
    unsigned int kernel_size = atoi(buf);
    uart_puts("Start to load the kernel image... \r\n");
    for (unsigned int i = 0; i < kernel_size; i++) {
        kernel_start[i] = uart_getc();
        uart_send('.');
    }
    uart_puts("after load");
    uart_puts("$ ");
    void (*jump)(void) = (void (*)(void))0x80000;
    jump();

    /*
    void(*jump)(char *) = (void (*)(char *))0x80000;
    jump(dtb_ptr);*/
}
void main() {
    uart_init();
    uart_puts("Bootloader Started!\n");
    load_kernel();
};
