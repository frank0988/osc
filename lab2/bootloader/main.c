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
    unsigned char *kernel_start = (unsigned char *)0x80000;
    char           buf[64];
    unsigned int   chunk_size = 256;
    unsigned int   received   = 0;
    uart_readline(buf);
    unsigned int kernel_size = atoi(buf);
    uart_puts("Start to load the kernel image... \r\n");
    while (received < kernel_size) {
        unsigned int to_receive = (kernel_size - received < chunk_size) ? (kernel_size - received) : chunk_size;

        for (unsigned int i = 0; i < to_receive; i++) {
            kernel_start[received + i] = uart_getc();
        }
        received += to_receive;

        uart_send('K');
    }
    uart_puts("\r\n");
    for (int i = 0; i < 1000; i++) asm volatile("nop");
    asm volatile("dsb sy");
    asm volatile("ic ialluis");
    asm volatile("dsb sy");
    asm volatile("isb");
    void (*jump)(void) = (void (*)(void))0x80000;
    jump();

    /*
    void(*jump)(char *) = (void (*)(char *))0x80000;
    jump(dtb_ptr);*/
}
int main() {
    uart_init();
    uart_puts("Bootloader Started!\n");
    load_kernel();
}
