.section ".text"
.global _start
_start:
    mov x0, 0
1:  add x0, x0, 1
    svc 0           // 觸發異常回到 Kernel
    cmp x0, 5
    blt 1b
1:  b 1b             // 結束後死循環