#include "thread.h"
#include <stddef.h>
#include "allocator.h"

// 全域變數定義
struct thread *run_queue = NULL; 
struct thread *current_thread = NULL;

static long thread_cnt = 0;

long generate_new_pid(void) {
    return thread_cnt++;
}

// 將 thread 加入 run queue (尾插法)
void add_to_runqueue(struct thread *t) {
    if (!t) return;
    
    t->next = NULL;
    t->prev = NULL;
    
    if (!run_queue) {
        run_queue = t;
    } else {
        struct thread *tail = run_queue;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = t;
        t->prev = tail;
    }
}

// 從 run queue 移除 thread
void remove_from_runqueue(struct thread *t) {
    if (!t) return;
    
    if (t->prev) {
        t->prev->next = t->next;
    } else {
        run_queue = t->next;
    }
    
    if (t->next) {
        t->next->prev = t->prev;
    }
    
    t->next = NULL;
    t->prev = NULL;
}

// Kernel thread wrapper: 確保 func() 返回後一定呼叫 thread_exit()
static void kernel_thread_wrapper(void) {
    struct thread *self = get_current();
    void (*func)(void) = (void (*)(void))self->context.x19;
    func();
    thread_exit();
}

struct thread *thread_create(void (*func)(void)) {
    struct thread *t = (struct thread *)kmalloc(sizeof(struct thread));
    if (!t) return NULL;
    
    t->stack = (char *)kmalloc(THREAD_STACK_SIZE);
    if (!t->stack) {
        kfree(t);
        return NULL;
    }
    
    // 初始化 context
    mm_zero(&t->context, sizeof(struct cpu_context));
    t->context.lr = (unsigned long)kernel_thread_wrapper;
    t->context.x19 = (unsigned long)func;  // 透過 callee-saved register 傳遞函數指標
    unsigned long sp = (unsigned long)(t->stack + THREAD_STACK_SIZE);
    t->context.sp = sp & ~0xFUL;  // 強制 16-byte 對齊
    t->context.fp = t->context.sp;

    t->status = THREAD_READY;
    t->preempt_count = 0;
    t->id = generate_new_pid();
    t->user_stack = NULL;  // kernel thread 不需要 user stack
    t->next = NULL;
    t->prev = NULL;

    // 加入 run queue（關中斷保護 linked list）
    unsigned long daif;
    asm volatile("mrs %0, daif" : "=r"(daif));
    asm volatile("msr daifset, #2" ::: "memory");
    add_to_runqueue(t);
    asm volatile("msr daif, %0" :: "r"(daif) : "memory");
    
    return t;
}

// User entry trampoline: 當 thread 被排程到時，從這裡進入 EL0
// x19 存有 user function 的地址（callee-saved，switch_to 會恢復）
static void user_entry_trampoline(void) {
    struct thread *self = get_current();
    unsigned long entry = self->context.x19;  // user function 的地址
    unsigned long user_sp = (unsigned long)(self->user_stack + USER_STACK_SIZE);
    
    // 從 EL1 切換到 EL0，不會返回
    from_el1_to_el0(entry, user_sp);
}

// 建立 User Mode Thread
struct thread *thread_create_user(void (*func)(void)) {
    struct thread *t = (struct thread *)kmalloc(sizeof(struct thread));
    if (!t) return NULL;
    
    // 分配 kernel stack (用於 exception 時保存狀態)
    t->stack = (char *)kmalloc(THREAD_STACK_SIZE);
    if (!t->stack) {
        kfree(t);
        return NULL;
    }
    
    // 分配 user stack
    t->user_stack = (char *)kmalloc(USER_STACK_SIZE);
    if (!t->user_stack) {
        kfree(t->stack);
        kfree(t);
        return NULL;
    }
    
    // 初始化 context
    mm_zero(&t->context, sizeof(struct cpu_context));
    t->context.lr = (unsigned long)user_entry_trampoline;
    unsigned long usp = (unsigned long)(t->stack + THREAD_STACK_SIZE);
    t->context.sp = usp & ~0xFUL;  // 強制 16-byte 對齊
    t->context.fp = t->context.sp;
    t->context.x19 = (unsigned long)func;  // 將 user function 地址存入 x19

    t->status = THREAD_READY;
    t->preempt_count = 0;
    t->id = generate_new_pid();
    t->next = NULL;
    t->prev = NULL;

    // 加入 run queue（關中斷保護 linked list）
    unsigned long daif2;
    asm volatile("mrs %0, daif" : "=r"(daif2));
    asm volatile("msr daifset, #2" ::: "memory");
    add_to_runqueue(t);
    asm volatile("msr daif, %0" :: "r"(daif2) : "memory");
    
    return t;
}

// Thread 結束時呼叫
void thread_exit(void) {
    if (current_thread) {
        current_thread->status = THREAD_DEAD;
        schedule();
    }
    // 不該執行到這裡
    while (1) {}
}