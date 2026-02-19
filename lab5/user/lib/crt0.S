// User-space _start entry point
// 呼叫 main()，結束後呼叫 exit()

.section ".text.entry"
.global _start
_start:
    bl main
    // main 返回後，呼叫 exit syscall
    mov x8, #5      // SYS_EXIT
    svc #0
    // fallback: 不應到達
    b .
