#define MAX_CMD_LEN 256
#include "mailbox.h"
#include "uart.h"
#include "cpio.h"
#include "reset.h"
void do_help(int argc, char **argv) {
    uart_puts("Available commands: help, hello, reboot, hw_info ,cpio_ls,cpio_cat\n");
};
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
void run_user_program(char* program_name) {
    void* program_addr = cpio_get_file(program_name); // 你之前寫過的 cpio 函數
    
    // 為 User Program 分配一個獨立的 Stack (簡單起見可以先用靜態陣列)
    static char user_stack[4096];
    
    uart_send_string("Jumping to EL0...\r\n");
    
    // 跳轉到 EL0，傳入程式地址與 stack 頂端 (往下長)
    move_to_user_mode(program_addr, user_stack + 4096);
}
void do_cpio_cat(int argc, char **argv) { 
    if (argc < 2) {
        uart_puts("Usage: cat <filename>\n");
        return;
    }
    cpio_cat(CPIO_DEFAULT_ADDR, argv[1]); 
}

typedef struct command {
    const char *name;
    const char *help;
    void (*func)(int argc, char **argv);
} command_t;
static command_t cmd_table[] = {{"help", "display coomands", do_help},
                                {"hello", "say hello", do_hello},
                                {"reboot", "reboot the device", do_reboot},
                                {"hw_info", "List hardware info", do_hw_info},
                                {"cpio_ls", "List file in CPIO", do_cpio_ls},
                                {"cpio_cat", "a", do_cpio_cat},
                                {"run", "run a user program", do_run_user_program},
                                {0, 0, 0}};

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
