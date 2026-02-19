#define MAX_CMD_LEN 256
#include "mailbox.h"
#include "uart.h"
#include "cpio.h"
#include "reset.h"
#include "shell.h"
#include "timer.h"
#include "allocator.h"
#include "thread.h"
static command_t cmd_table[] = {{"help", "display coomands", do_help},
                                {"hello", "say hello", do_hello},
                                {"reboot", "reboot the device", do_reboot},
                                {"hw_info", "List hardware info", do_hw_info},
                                {"cpio_ls", "List file in CPIO", do_cpio_ls},
                                {"cpio_cat", "a", do_cpio_cat},
                                {"run", "run a user program", do_run_user_program},
                                {"settimeout", "set a timer to print message after seconds", do_timeout},
                                {"mm_test","test allocator",do_mm_test},
                                {0, 0, 0}};

void do_help(int argc, char **argv) {
    uart_puts("Available commands:\n");
    
    for (int i = 0; cmd_table[i].name != 0; i++) {
        uart_printf(" - %s", cmd_table[i].name);
        
        // 如果有描述資訊，也順便印出來
        if (cmd_table[i].help != 0) {
            uart_printf(": %s", cmd_table[i].help);
        }
        uart_puts("\n");
    }
}
void do_hello(int argc, char **argv) { uart_puts("Hello World!\n"); };
void do_reboot(int argc, char **argv) {
    uart_puts("reboot\n");
    reset(10);
    while (1) {
    }
};
void do_hw_info(int argc, char **argv) {
    get_board_revision();
    get_arm_memory();
};
void do_cpio_ls(int argc, char **argv) { 
    cpio_ls(CPIO_DEFAULT_ADDR); 
}
void do_run_user_program(int argc, char **argv) {
    if (argc < 2) {
        uart_puts("Usage: run <program_name>\n");
        return;
    }
    run_user_program(argv[1]);
}

// 用於傳遞 user program 資訊給新 thread
static const char *_user_prog_name = NULL;

// 新 thread 的入口函式：載入 user program 並跳到 EL0
static void user_program_entry(void) {
    extern const char* cpio_get_file(void *addr, const char *filename, unsigned int *out_size);
    
    unsigned int program_size = 0;
    const char *program_addr = cpio_get_file(CPIO_DEFAULT_ADDR, _user_prog_name, &program_size);
    
    if (!program_addr) {
        thread_exit();
        return;
    }
    
    // 分配 user stack
    char *ustack = (char *)kmalloc(USER_STACK_SIZE);
    if (!ustack) {
        thread_exit();
        return;
    }
    current_thread->user_stack = ustack;
    
    // 複製程式碼到對齊的 buffer
    char *code_buf = (char *)kmalloc(program_size);
    if (!code_buf) {
        kfree(ustack);
        thread_exit();
        return;
    }
    for (unsigned int i = 0; i < program_size; i++) {
        code_buf[i] = program_addr[i];
    }
    
    // 跳到 EL0 執行 user program（不會返回）
    from_el1_to_el0((unsigned long)code_buf, (unsigned long)(ustack + USER_STACK_SIZE));
}

void run_user_program(const char* program_name) {
    extern const char* cpio_get_file(void *addr, const char *filename, unsigned int *out_size);
    
    // 先確認程式存在
    unsigned int program_size = 0;
    const char *program_addr = cpio_get_file(CPIO_DEFAULT_ADDR, program_name, &program_size);
    
    if (!program_addr) {
        uart_puts("Error: Program not found\n");
        return;
    }
    
    uart_puts("Loading user program: ");
    uart_puts(program_name);
    uart_puts(" (");
    uart_hex(program_size);
    uart_puts(" bytes)\n");
    
    // 設定全域變數，讓新 thread 知道要載入哪個程式
    _user_prog_name = program_name;
    
    // 建立新的 kernel thread 來執行 user program
    struct thread *t = thread_create(user_program_entry);
    if (!t) {
        uart_puts("Error: Failed to create thread\n");
        return;
    }
    
    uart_printf_polling("[DEBUG] Created thread PID=%d, status=%d\n", t->id, t->status);
    uart_printf_polling("[DEBUG] Run queue: ");
    struct thread *tmp = run_queue;
    while (tmp) {
        uart_printf_polling("PID%d(s=%d) -> ", tmp->id, tmp->status);
        tmp = tmp->next;
    }
    uart_printf_polling("NULL\n");
    
    // Idle thread 等待：不停讓出 CPU，直到所有 user threads 都結束
    while (has_user_threads()) {
        kill_zombies();
        schedule();
    }
    
    // 清理 zombie threads
    kill_zombies();
}
void do_cpio_cat(int argc, char **argv) { 
    if (argc < 2) {
        uart_puts("Usage: cat <filename>\n");
        return;
    }
    cpio_cat(CPIO_DEFAULT_ADDR, argv[1]); 
}

// 定時器回調函數：arg 指向 event->data，資料生命週期由 timer 管理
static void timeout_callback(void *arg) {
    uart_puts((char *)arg);
    uart_puts("\n");
}

void do_timeout(int argc, char **argv) {
    if (argc < 2) {
        uart_puts("Usage: settimeout <seconds> [message]\n");
        return;
    }
    int seconds = atoi(argv[1]);
    const char *message = (argc >= 3) ? argv[2] : "Timeout!";
    
    // 使用 timer_add_event_str 複製字串，避免 argv 被覆蓋
    timer_add_event_str(
        timeout_callback,
        message,
        (uint64_t)seconds * 1000
    );
    
    uart_printf("Timer set for %d seconds\n", seconds);
}
void do_mm_test(int argc,char **argv){
    mm_test();
}
void shell_execute(char *cmd_buffer) {
    char *argv[16];  // max 16 words
    int   argc = 0;

    // tokenize
    char *token = strtok(cmd_buffer, " ");
    while (token != NULL && argc < 16) {
        argv[argc++] = token;
        token        = strtok(NULL, " ");
    }

    if (argc == 0) return;

    for (int i = 0; cmd_table[i].name != NULL; i++) {
        if (strcmp(argv[0], cmd_table[i].name) == 0) {
            cmd_table[i].func(argc, argv);
            return;
        }
    }

    uart_puts("Unknown command\n");
}

int shell() {
    
    uart_puts("Welcome to the shell\n");
    char cmd_buffer[MAX_CMD_LEN];
    while (1) {
        uart_puts("# ");
        uart_readline(cmd_buffer, MAX_CMD_LEN);
        shell_execute(cmd_buffer);
    }
    return 0;
}
