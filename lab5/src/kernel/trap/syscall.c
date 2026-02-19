#include "syscall.h"
#include "thread.h"
#include "uart.h"
#include "cpio.h"
#include "allocator.h"
#include "mailbox.h"

extern void *CPIO_DEFAULT_ADDR;

// 來自 thread.c
extern long generate_new_pid(void);

// assembly: 從 kernel stack 上的 trap frame 恢復並 eret 回 EL0
extern void ret_from_fork(void);

// ============================================================
// 個別 syscall 實作
// ============================================================

// SYS_GETPID: 回傳 current thread id
static long sys_getpid(void) {
    return current_thread->id;
}

// SYS_UART_READ: 從 UART 讀取 size bytes 到 buf
static long sys_uart_read(char *buf, unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        buf[i] = uart_getc();
    }
    return (long)size;
}

// SYS_UART_WRITE: 將 buf 中 size bytes 寫入 UART
// 注意：使用 polling 版本，因為 SVC handler 中 IRQ 可能被禁用
static long sys_uart_write(const char *buf, unsigned long size) {
    /*
    uart_printf_polling("[uart_write] buf=%p size=%lu first4: %x %x %x %x\n",
        (unsigned long)buf, size,
        size > 0 ? (unsigned int)(unsigned char)buf[0] : 0,
        size > 1 ? (unsigned int)(unsigned char)buf[1] : 0,
        size > 2 ? (unsigned int)(unsigned char)buf[2] : 0,
        size > 3 ? (unsigned int)(unsigned char)buf[3] : 0);
    */
    for (unsigned long i = 0; i < size; i++) {
        if (buf[i] == '\n') uart_send_polling('\r');
        uart_send_polling((unsigned int)buf[i]);
    }
    return (long)size;
}

// SYS_EXEC: 從 CPIO 載入程式並替換當前 process
static long sys_exec(struct trap_frame *tf, const char *name) {
    unsigned int program_size = 0;
    const char *program_addr = cpio_get_file(CPIO_DEFAULT_ADDR, name, &program_size);
    
    if (!program_addr) {
        return -1;  // 找不到程式
    }
    
    // 分配新的 user stack（如果已有就重用）
    if (!current_thread->user_stack) {
        current_thread->user_stack = (char *)kmalloc(USER_STACK_SIZE);
        if (!current_thread->user_stack) {
            return -1;  // 記憶體不足
        }
    }
    
    // Allocate aligned code buffer
    char *code_buf = (char *)kmalloc(program_size);
    if (!code_buf) {
        return -1;
    }
    // Copy code
    for (unsigned int i = 0; i < program_size; i++) {
        code_buf[i] = program_addr[i];
    }
    
    // 修改 trap frame，eret 後就會跳到新程式
    tf->elr_el1 = (unsigned long)code_buf;
    tf->sp_el0 = (unsigned long)(current_thread->user_stack + USER_STACK_SIZE);
    
    // 清除所有通用暫存器（給新程式乾淨的環境）
    for (int i = 0; i < 31; i++) {
        tf->regs[i] = 0;
    }
    
    return 0;  // 不會真正返回，因為 eret 會到新程式
}

// SYS_FORK: 複製 process
static long sys_fork(struct trap_frame *parent_tf) {
    // 1. 建立 child thread
    struct thread *child = (struct thread *)kmalloc(sizeof(struct thread));
    if (!child) return -1;
    
    child->stack = (char *)kmalloc(THREAD_STACK_SIZE);
    if (!child->stack) {
        kfree(child);
        return -1;
    }
    
    child->user_stack = (char *)kmalloc(USER_STACK_SIZE);
    if (!child->user_stack) {
        kfree(child->stack);
        kfree(child);
        return -1;
    }
    
    // 2. 複製 parent 的 user stack 內容
    char *parent_user_stack = current_thread->user_stack;
    if (parent_user_stack) {
        for (int i = 0; i < USER_STACK_SIZE; i++) {
            child->user_stack[i] = parent_user_stack[i];
        }
    }
    
    // 3. 在 child 的 kernel stack 頂部放置 trap frame 副本
    //    child 被排程到時，ret_from_fork 會從這裡 load_all + eret
    struct trap_frame *child_tf = 
        (struct trap_frame *)(child->stack + THREAD_STACK_SIZE - sizeof(struct trap_frame));
    
    // 複製 parent 的 trap frame
    for (unsigned long i = 0; i < sizeof(struct trap_frame) / sizeof(unsigned long); i++) {
        ((unsigned long *)child_tf)[i] = ((unsigned long *)parent_tf)[i];
    }
    
    // 4. 調整 child 的 trap frame
    child_tf->regs[0] = 0;  // child 的 fork 返回值 = 0
    
    // 修正 sp_el0 和 fp (x29): 計算 parent 在 user stack 中的 offset，套用到 child 的 user stack
    if (parent_user_stack) {
        unsigned long parent_stack_base = (unsigned long)parent_user_stack;
        unsigned long parent_stack_top  = parent_stack_base + USER_STACK_SIZE;
        unsigned long child_stack_top   = (unsigned long)(child->user_stack + USER_STACK_SIZE);

        // 修正 sp_el0
        unsigned long sp_offset = parent_stack_top - parent_tf->sp_el0;
        child_tf->sp_el0 = child_stack_top - sp_offset;

        // 修正 fp (x29)：若 fp 落在 parent user stack 範圍內，也要重新定位
        unsigned long parent_fp = parent_tf->regs[29];
        if (parent_fp >= parent_stack_base && parent_fp < parent_stack_top) {
            unsigned long fp_offset = parent_stack_top - parent_fp;
            child_tf->regs[29] = child_stack_top - fp_offset;
        }
    }
    
    // 5. 設定 child 的 cpu_context
    mm_zero(&child->context, sizeof(struct cpu_context));
    child->context.lr = (unsigned long)ret_from_fork;  // 排程到時從 ret_from_fork 開始
    child->context.sp = (unsigned long)child_tf;        // sp 指向 trap frame 位置
    child->context.fp = child->context.sp;
    
    // 6. 初始化 child 的其他欄位
    child->status = THREAD_READY;
    child->preempt_count = 0;
    child->id = generate_new_pid();
    child->next = NULL;
    child->prev = NULL;
    
    // 7. 加入 run queue（關中斷保護 linked list）
    unsigned long daif;
    asm volatile("mrs %0, daif" : "=r"(daif));
    asm volatile("msr daifset, #2" ::: "memory");
    add_to_runqueue(child);
    asm volatile("msr daif, %0" :: "r"(daif) : "memory");
    
    // 8. parent 的返回值 = child PID
    return child->id;
}

// SYS_EXIT: 終止 process
static void sys_exit(void) {
    current_thread->status = THREAD_DEAD;
    schedule();
    // 不應到達這裡
    while (1) {}
}

// SYS_MBOX_CALL: 代替 user process 呼叫 mailbox
static long sys_mbox_call(unsigned char ch, unsigned int *mbox_user) {
    // 從 user buffer 讀取 size (第一個 word，單位是 bytes)
    unsigned int size = mbox_user[0];
    unsigned int count = size / 4;  // 轉成 word count
    if (count > 36) count = 36;     // 限制不超過 mbox buffer 大小

    // 從 user space 複製到 kernel 的全域 mbox buffer
    for (unsigned int i = 0; i < count; i++) {
        mbox[i] = mbox_user[i];
    }

    // 呼叫 kernel 的 mbox_call
    int ret = mbox_call(ch);

    // 將結果複製回 user space
    for (unsigned int i = 0; i < count; i++) {
        mbox_user[i] = mbox[i];
    }

    return ret;
}

// SYS_KILL: 殺掉指定 PID 的 thread
static void sys_kill(int pid) {
    struct thread *t = run_queue;
    while (t) {
        if (t->id == pid) {
            t->status = THREAD_DEAD;
            return;
        }
        t = t->next;
    }
    // PID not found, silently ignore
}

// ============================================================
// Syscall Dispatcher
// ============================================================

void syscall_dispatch(struct trap_frame *tf) {
    unsigned long syscall_num = tf->regs[8];  // x8 = syscall number
    
    switch (syscall_num) {
        case SYS_GETPID:
            tf->regs[0] = sys_getpid();
            break;
            
        case SYS_UART_READ:
            tf->regs[0] = sys_uart_read((char *)tf->regs[0], tf->regs[1]);
            break;
            
        case SYS_UART_WRITE:
            tf->regs[0] = sys_uart_write((const char *)tf->regs[0], tf->regs[1]);
            break;
            
        case SYS_EXEC:
            tf->regs[0] = sys_exec(tf, (const char *)tf->regs[0]);
            break;
            
        case SYS_FORK:
            tf->regs[0] = sys_fork(tf);
            break;
            
        case SYS_EXIT:
            sys_exit();
            break;

        case SYS_MBOX_CALL:
            tf->regs[0] = sys_mbox_call((unsigned char)tf->regs[0],
                                         (unsigned int *)tf->regs[1]);
            break;

        case SYS_KILL:
            sys_kill((int)tf->regs[0]);
            break;
            
        default:
            uart_printf_polling("[SYSCALL] Unknown syscall number: %ld\n", syscall_num);
            tf->regs[0] = -1;
            break;
    }
}
