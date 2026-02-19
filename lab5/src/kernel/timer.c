#include "timer.h"

#define SCHED_QUANTUM_MS 30  // 排程時間片：30ms

// 全局變量定義
volatile int timer_count = 0;
timer_event_t *timer_queue = NULL;
uint64_t system_time = 0;

// 靜態記憶體池
static timer_event_t timer_pool[MAX_TIMER_EVENTS];
static int timer_pool_initialized = 0;

// 用於保護 Critical Section 的 IRQ 開關
static inline void disable_irq(void) {
    asm volatile("msr daifset, #2" ::: "memory");
}

static inline void enable_irq(void) {
    asm volatile("msr daifclr, #2" ::: "memory");
}

// 初始化記憶體池
static void init_timer_pool(void) {
    if (!timer_pool_initialized) {
        for (int i = 0; i < MAX_TIMER_EVENTS; i++) {
            timer_pool[i].in_use = 0;
        }
        timer_pool_initialized = 1;
    }
}

// 從記體池分配一個 timer_event
static timer_event_t* alloc_timer_event(void) {
    init_timer_pool();
    
    for (int i = 0; i < MAX_TIMER_EVENTS; i++) {
        if (!timer_pool[i].in_use) {
            timer_pool[i].in_use = 1;
            return &timer_pool[i];
        }
    }
    return NULL; // 記體池已滿
}

// 釋放 timer_event 回記憂體池
static void free_timer_event(timer_event_t *event) {
    if (event >= timer_pool && event < timer_pool + MAX_TIMER_EVENTS) {
        event->in_use = 0;
    }
}

void timer_init(void){
    // Allow EL0 to access physical timer registers (cntpct_el0, cntfrq_el0)
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));

    // Enable core timer and unmask IRQ
    core_timer_enable();
    asm volatile("msr daifclr, #2");
}

void handle_core_irq() {
    timer_count++;
    system_time = timer_get_current_time();
    
    // 處理所有到期的事件
    while (timer_queue && timer_queue->expire_time <= system_time) {
        timer_event_t *event = timer_pop_event();
        if (event->callback) {
            event->callback(event->args);
        }
        free_timer_event(event);
    }
    
    // 設置下一個中斷時間（不超過排程時間片）
    if (timer_queue) {
        uint64_t remaining = timer_queue->expire_time - system_time;
        if (remaining == 0) remaining = 1;
        // 確保不超過排程時間片，讓 preemptive scheduling 能正常運作
        if (remaining > SCHED_QUANTUM_MS) remaining = SCHED_QUANTUM_MS;
        timer_set_next_interrupt(remaining);
    } else {
        // 隊列為空時，以排程時間片為間隔持續觸發中斷
        timer_set_next_interrupt(SCHED_QUANTUM_MS);
    }
}

uint64_t timer_get_current_time(void){
    uint64_t frequency = get_cntfrq_el0();
    uint64_t count = get_cntpct_el0();
    return (count * 1000) / frequency; // 返回毫秒
}

// 添加定時器事件到優先隊列（按到期時間排序）
// 注意：args 是使用者傳入的指標，不會被複製。使用者需確保 args 指向的資料生命週期足夠長。
void timer_add_event(void (*callback)(void*), void *args, uint64_t duration) {
    // 關閉中斷，保護 Critical Section
    disable_irq();
    
    timer_event_t *new_event = alloc_timer_event();
    if (!new_event) {
        enable_irq();
        return; // 記憶體池已滿
    }
    
    // 直接儲存指標，不複製內容
    // 如果需要複製資料，應使用 timer_add_event_copy 或要求使用者傳入長度
    new_event->args = args;
    
    uint64_t current_time = timer_get_current_time();
    new_event->expire_time = current_time + duration;
    new_event->callback = callback;
    new_event->next = NULL;
    
    // 插入到優先隊列中（按到期時間排序）
    if (!timer_queue || timer_queue->expire_time > new_event->expire_time) {
        // 插入到隊首
        new_event->next = timer_queue;
        timer_queue = new_event;
        // 更新中斷時間
        timer_set_next_interrupt(duration);
    } else {
        // 找到合適的位置插入
        timer_event_t *current = timer_queue;
        while (current->next && current->next->expire_time <= new_event->expire_time) {
            current = current->next;
        }
        new_event->next = current->next;
        current->next = new_event;
    }
    
    enable_irq();
}

// 添加定時器事件（字串版本，會複製字串到內部 buffer）
// 適用於傳入可能被覆蓋的字串（如 argv）
void timer_add_event_str(void (*callback)(void*), const char *str, uint64_t duration) {
    disable_irq();
    
    timer_event_t *new_event = alloc_timer_event();
    if (!new_event) {
        enable_irq();
        return;
    }
    
    // 將字串複製到 event 內部的 data buffer
    if (str) {
        int i;
        for (i = 0; i < TIMER_DATA_SIZE - 1 && str[i]; i++)
            new_event->data[i] = str[i];
        new_event->data[i] = '\0';
        new_event->args = new_event->data;
    } else {
        new_event->args = NULL;
    }
    
    uint64_t current_time = timer_get_current_time();
    new_event->expire_time = current_time + duration;
    new_event->callback = callback;
    new_event->next = NULL;
    
    if (!timer_queue || timer_queue->expire_time > new_event->expire_time) {
        new_event->next = timer_queue;
        timer_queue = new_event;
        timer_set_next_interrupt(duration);
    } else {
        timer_event_t *current = timer_queue;
        while (current->next && current->next->expire_time <= new_event->expire_time) {
            current = current->next;
        }
        new_event->next = current->next;
        current->next = new_event;
    }
    
    enable_irq();
}

// 取消定時器事件
void timer_cancel_event(timer_event_t *event) {
    if (!timer_queue || !event) {
        return;
    }
    
    // 關閉中斷，保護 Critical Section
    disable_irq();
    
    if (timer_queue == event) {
        // 取消隊首事件
        timer_queue = event->next;
        free_timer_event(event);
        // 更新下一個中斷
        if (timer_queue) {
            uint64_t current_time = timer_get_current_time();
            if (timer_queue->expire_time > current_time) {
                timer_set_next_interrupt(timer_queue->expire_time - current_time);
            }
        }
    } else {
        // 在隊列中查找並刪除
        timer_event_t *current = timer_queue;
        while (current->next) {
            if (current->next == event) {
                current->next = event->next;
                free_timer_event(event);
                break;
            }
            current = current->next;
        }
    }
    
    enable_irq();
}

// 從隊列中彈出事件
timer_event_t* timer_pop_event(void) {
    if (!timer_queue) {
        return NULL;
    }
    
    timer_event_t *event = timer_queue;
    timer_queue = timer_queue->next;
    return event;
}

// 設置下一次中斷時間
void timer_set_next_interrupt(uint64_t duration) {
    // 將毫秒轉換為時鐘週期
    uint64_t frequency = get_cntfrq_el0();
    uint64_t ticks = (duration * frequency) / 1000;
    
    // 設置 cntp_tval_el0 寄存器（physical timer）
    asm volatile("msr cntp_tval_el0, %0" :: "r"(ticks));
}

// 定時器 tick 處理（每個 tick 更新系統時間）
void timer_tick_handler(void) {
    system_time = timer_get_current_time();
}

