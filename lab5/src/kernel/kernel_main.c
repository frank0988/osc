#include "shell.h"
#include "uart.h"
#include "dtb.h"
#include "timer.h"
#include "allocator.h"
#include "thread.h"
// 建立一個全域變數或靜態變數來存放解析結果（例如 Initrd 位址）
extern void *initrd_start; //define in dtb.c
extern void *CPIO_DEFAULT_ADDR; // define in cpio.c

void init(){
    uart_init();
    mm_init();
    timer_init();
    scheduler_init();
}


void kernel_main(void *dtb_ptr) {
    init();

    // 清除啟動雜訊
    while (*AUX_MU_LSR_REG & 0x01) { uart_getc(); }

    
    
    uart_printf("DTB address: %p\n", dtb_ptr);
    if (dtb_ptr) {
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
    
    // 記憶體分配器必須在啟用中斷之前初始化
    
    
    scheduler_start();
    // 所有 worker threads 結束後，schedule() 回到這裡 (PID 0)
    kill_zombies();
    uart_printf("All threads completed. Entering shell.\n");
    shell();
}