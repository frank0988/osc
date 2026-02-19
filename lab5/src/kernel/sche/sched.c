#include "thread.h"
#include "allocator.h"
#include "uart.h"

// Zombie queue - 等待被回收的 thread
static struct thread *zombie_queue = NULL;

// Idle thread 指標
struct thread *idle_thread_ptr = NULL;

// 將 thread 加入 zombie queue
static void add_to_zombie_queue(struct thread *t) {
    if (!t) return;
    t->next = zombie_queue;
    t->prev = NULL;
    if (zombie_queue) {
        zombie_queue->prev = t;
    }
    zombie_queue = t;
}

// 清理所有 zombie threads
void kill_zombies(void) {
    while (zombie_queue) {
        struct thread *t = zombie_queue;
        zombie_queue = t->next;
        if (zombie_queue) {
            zombie_queue->prev = NULL;
        }
        
        // 釋放資源
        if (t->user_stack) {
            kfree(t->user_stack);
        }
        if (t->stack) {
            kfree(t->stack);
        }
        kfree(t);
    }
}



// Preemption 控制（強定義，覆蓋 uart.c 中的 weak 版本）
void preempt_disable(void) {
    if (current_thread) current_thread->preempt_count++;
}

void preempt_enable(void) {
    if (current_thread && current_thread->preempt_count > 0)
        current_thread->preempt_count--;
}

// 簡單的 Round-Robin 排程器
void schedule(void) {
    // 保存中斷狀態並關閉 IRQ（保護 queue 操作 + 防止巢狀中斷）
    unsigned long daif_val;
    asm volatile("mrs %0, daif" : "=r"(daif_val));
    asm volatile("msr daifset, #2" ::: "memory");

    // 如果 preempt 被禁用且 thread 還活著，跳過搶佔
    if (current_thread && current_thread->preempt_count > 0
        && current_thread->status != THREAD_DEAD) {
        asm volatile("msr daif, %0" :: "r"(daif_val) : "memory");
        return;
    }

    struct thread *prev = current_thread;
    struct thread *next = NULL;
    
    // 處理當前 thread 的狀態
    if (prev) {
        if (prev->status == THREAD_DEAD) {
            // 將 dead thread 移到 zombie queue，不在這裡釋放
            remove_from_runqueue(prev);
            add_to_zombie_queue(prev);
        } else if (prev->status == THREAD_RUNNING) {
            // 將當前 thread 改為 READY
            prev->status = THREAD_READY;
        }
    }
    
    // 選擇下一個 READY 的 thread (從當前位置的下一個開始找)
    // 注意：如果 prev 已被移出 run queue (DEAD)，prev->next 指向 zombie chain，不能使用
    if (prev && prev->status != THREAD_DEAD && prev->next) {
        next = prev->next;
    } else {
        next = run_queue;
    }
    
    // 遍歷找 READY 的 thread
    struct thread *start = next;
    while (next) {
        if (next->status == THREAD_READY && next != idle_thread_ptr) {
            break;
        }
        next = next->next;
        if (!next) {
            next = run_queue; // 回到開頭
        }
        if (next == start) {
            // 繞完一圈，沒有找到其他可執行的 thread
            next = NULL;
            break;
        }
    }
    
    // 如果沒有其他 thread，選擇 idle thread
    if (!next) {
        next = idle_thread_ptr;
    }
    
    // 如果還是沒有 (不應該發生)，返回
    if (!next) {
        return;
    }
    
    // 如果 next 就是 prev 且 prev 是 READY，繼續執行
    if (next == prev) {
        prev->status = THREAD_RUNNING;
        return;
    }
    
    // 切換到下一個 thread
    next->status = THREAD_RUNNING;
    current_thread = next;
    
    // 執行 context switch
    if (prev) {
        switch_to(&prev->context, &next->context);
    }
    // 恢復呼叫 schedule() 時的中斷狀態
    asm volatile("msr daif, %0" :: "r"(daif_val) : "memory");
}

// 初始化排程器
// 將 kernel_main 註冊為 PID 0 (idle thread)，使用當前的 boot stack
void scheduler_init(void) {
    run_queue = NULL;
    zombie_queue = NULL;
    
    // 為 kernel_main 建立 thread 結構 (PID 0 / idle thread)
    // 不分配新 stack，因為 kernel_main 已經在 boot stack 上執行
    idle_thread_ptr = (struct thread *)kmalloc(sizeof(struct thread));
    mm_zero(&idle_thread_ptr->context, sizeof(struct cpu_context));
    idle_thread_ptr->id = generate_new_pid();  // PID 0, and increments thread_cnt
    idle_thread_ptr->status = THREAD_RUNNING;  // 它已經在執行了
    idle_thread_ptr->preempt_count = 0;
    idle_thread_ptr->stack = NULL;  // boot stack，不要 free
    idle_thread_ptr->user_stack = NULL;  // kernel idle thread，沒有 user stack
    idle_thread_ptr->next = NULL;
    idle_thread_ptr->prev = NULL;
    
    // 加入 run queue 並設為當前 thread
    add_to_runqueue(idle_thread_ptr);
    current_thread = idle_thread_ptr;
}

// 啟動排程器 - context switch 到第一個 worker thread
// 當所有 worker threads 結束後，schedule() 會把 CPU 交回 idle_thread_ptr (kernel_main)
void scheduler_start(void) {
    if (run_queue) {
        schedule();
    }
}

// 檢查是否還有任何非 idle 的 user thread 正在執行或等待執行
int has_user_threads(void) {
    struct thread *t = run_queue;
    while (t) {
        if (t != idle_thread_ptr && t != current_thread && 
            (t->status == THREAD_READY || t->status == THREAD_RUNNING)) {
            return 1;
        }
        t = t->next;
    }
    return 0;
}

