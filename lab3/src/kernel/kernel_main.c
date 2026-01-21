#include "shell.h"
#include "uart.h"
#include "dtb.h"  // 包含你剛剛寫好的 Parser

// 建立一個全域變數或靜態變數來存放解析結果（例如 Initrd 位址）
extern void *initrd_start; //define in dtb.c
extern void *CPIO_DEFAULT_ADDR; // define in cpio.c



void kernel_main(void *dtb_ptr) {
    uart_init();

    // 清除啟動雜訊
    while (*AUX_MU_LSR_REG & 0x01) { uart_getc(); }

    uart_puts("Kernel initialized.\n");
    
    // 改用組合拳印出位址
    
    uart_puts("DTB address: 0x");
    uart_put_hex((uint64_t)dtb_ptr);
    uart_puts("\n");
    
    uart_printf("DTB address: %p\n", dtb_ptr);
    if (dtb_ptr) {
        uart_puts("dtb test\n");
        //dtb_traverse(dtb_ptr, debug_dump_callback);
        dtb_traverse(dtb_ptr, dtb_callback_find_initrd);
        
    }

    if (initrd_start) {
        uart_puts("Initrd detected at: 0x");
        CPIO_DEFAULT_ADDR = initrd_start;
        uart_put_hex((uint64_t)initrd_start);
        uart_puts("\n");
    } else {
        uart_puts("Initrd not found in DTB.\n");
    }
    asm volatile("svc #1"); 
    uart_puts("Press any key to start shell...\n");
    uart_getc();
    
    shell();
}