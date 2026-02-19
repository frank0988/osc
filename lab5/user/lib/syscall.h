// User-side syscall wrappers
// 這些函式可以在 EL0 的 C 程式中直接呼叫

#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

// Syscall numbers (must match kernel's syscall.h)
#define SYS_GETPID      0
#define SYS_UART_READ   1
#define SYS_UART_WRITE  2
#define SYS_EXEC        3
#define SYS_FORK        4
#define SYS_EXIT        5
#define SYS_MBOX_CALL   6
#define SYS_KILL        7

int get_pid(void);
int fork(void);
void exit(void);
long uart_read(char *buf, long size);
long uart_write(const char *buf, long size);
int exec(const char *name);
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);

// printf (implemented in printf.c)
int printf(const char *fmt, ...);

// utility
void delay(long count);

#endif
