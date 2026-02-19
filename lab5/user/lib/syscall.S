// User-space syscall wrappers
// 每個 wrapper: 把 syscall number 放入 x8, 參數放 x0-x5, 執行 svc #0

.global get_pid
get_pid:
    mov x8, #0          // SYS_GETPID
    svc #0
    ret

.global uart_read
uart_read:
    // x0 = buf, x1 = size (already in place)
    mov x8, #1          // SYS_UART_READ
    svc #0
    ret

.global uart_write
uart_write:
    // x0 = buf, x1 = size (already in place)
    mov x8, #2          // SYS_UART_WRITE
    svc #0
    ret

.global exec
exec:
    // x0 = name (already in place)
    mov x8, #3          // SYS_EXEC
    svc #0
    ret

.global fork
fork:
    mov x8, #4          // SYS_FORK
    svc #0
    ret

.global exit
exit:
    mov x8, #5          // SYS_EXIT
    svc #0
    // 不會返回
    b exit

.global mbox_call
mbox_call:
    // x0 = ch, x1 = mbox (already in place)
    mov x8, #6          // SYS_MBOX_CALL
    svc #0
    ret

.global kill
kill:
    // x0 = pid (already in place)
    mov x8, #7          // SYS_KILL
    svc #0
    ret

.global delay
delay:
    // x0 = count
    subs x0, x0, #1
    bgt delay
    ret
