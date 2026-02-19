#ifndef SYSCALL_H
#define SYSCALL_H

#include "thread.h"

// Syscall numbers
#define SYS_GETPID      0
#define SYS_UART_READ   1
#define SYS_UART_WRITE  2
#define SYS_EXEC        3
#define SYS_FORK        4
#define SYS_EXIT        5
#define SYS_MBOX_CALL   6
#define SYS_KILL        7

// Syscall dispatcher (called from EL0 sync handler)
void syscall_dispatch(struct trap_frame *tf);

#endif
