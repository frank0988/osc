#define MAX_CMD_LEN 256
#include "mailbox.h"
#include "uart.h"
#include "cpio.h"
#include "reset.h"
#include "shell.h"
#include "timer.h"

static command_t cmd_table[] = {{"help", "display coomands", do_help},
                                {"hello", "say hello", do_hello},
                                {"reboot", "reboot the device", do_reboot},
                                {"hw_info", "List hardware info", do_hw_info},
                                {"cpio_ls", "List file in CPIO", do_cpio_ls},
                                {"cpio_cat", "a", do_cpio_cat},
                                {"run", "run a user program", do_run_user_program},
                                {"settimeout", "set a timer to print message after seconds", do_timeout},
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
void run_user_program(const char* program_name) {
    // 从 CPIO 找到用户程序
    extern const char* cpio_get_file(void *addr, const char *filename, unsigned int *out_size);
    
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
    
    // 配置用户栈（静态分配 4KB）
    static char user_stack[4096] __attribute__((aligned(16)));
    void *user_stack_top = user_stack + sizeof(user_stack);
    
    // 调用汇编函数跳转到 EL0
    // from_el1_to_el0(程序入口地址, 用户栈指针)
    extern void from_el1_to_el0(void *entry, void *sp);
    from_el1_to_el0((void *)program_addr, user_stack_top);
    
    // 如果用户程序返回（不应该发生）
    uart_puts("User program returned unexpectedly\n");
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
    
    // timer_add_event 會把 message 複製到 event->data 中
    timer_add_event(
        timeout_callback,
        (void *)message,
        (uint64_t)seconds * 1000
    );
    
    uart_printf("Timer set for %d seconds\n", seconds);
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
