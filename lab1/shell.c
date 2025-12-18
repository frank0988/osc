#define MAX_CMD_LEN 256
#include "mailbox.h"
#include "uart.h"
int main() {
  uart_init();
  uart_puts("Welcome");
  char cmd_buffer[MAX_CMD_LEN];
  while (1) {
    uart_puts("# ");
    int idx = 0;
    while (1) {
      char c = uart_getc();
      if (c == '\n' || c == '\r') {
        uart_puts("\n");
        cmd_buffer[idx] = '\0';
        break;
      }
      // 處理 Backspace (Delete)
      // 127 是 DEL, 8 是 Backspace，不同 terminal 送出的碼不同
      else if (c == 127 || c == 8) {
        if (idx > 0) {
          idx--;
          // 視覺上的刪除：倒退一格 -> 印空白蓋掉 -> 再倒退一格
          uart_puts("\b \b");
        }
      } else {
        if (idx < MAX_CMD_LEN - 1) {
          cmd_buffer[idx++] = c;
          uart_send(c);
        }
      }
    }

    if (strcmp(cmd_buffer, "help") == 0) {
      uart_puts("Available commands: help, hello, reboot\n");
    } else if (strcmp(cmd_buffer, "hello") == 0) {
      uart_puts("Hello World!\n");
    } else if (strcmp(cmd_buffer, "reboot") == 0) {
      uart_puts("Rebooting...\n");
    } else if (strcmp(cmd_buffer, "hw info") == 0) {
      get_board_revision();
      get_arm_memory();
    }

    else if (idx > 0) {
      uart_puts("Unknown command: ");
      uart_puts(cmd_buffer);
      uart_puts("\n");
    }
  }
}
