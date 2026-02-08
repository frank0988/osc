#include "timer.h"

// 全局變量定義
volatile int timer_count = 0;
timer_event_t *timer_queue = NULL;
uint64_t system_time = 0;

// 靜態記憂體池
static timer_event_t timer_pool[MAX_TIMER_EVENTS];
static int timer_pool_initialized = 0;

// 初始化記憂體池
static void init_timer_pool(void) {
    if (!timer_pool_initialized) {
        for (int i = 0; i < MAX_TIMER_EVENTS; i++) {
            timer_pool[i].in_use = 0;
        }
        timer_pool_initialized = 1;
    }
}

// 從記憂體池分配一個 timer_event
static timer_event_t* alloc_timer_event(void) {
    init_timer_pool();
    
    for (int i = 0; i < MAX_TIMER_EVENTS; i++) {
        if (!timer_pool[i].in_use) {
            timer_pool[i].in_use = 1;
            return &timer_pool[i];
        }
    }
    return NULL; // 記憂體池已滿
}

// 釋放 timer_event 回記憂體池
static void free_timer_event(timer_event_t *event) {
    if (event >= timer_pool && event < timer_pool + MAX_TIMER_EVENTS) {
        event->in_use = 0;
    }
}

void timer_init(void){
    // Enable core timer and unmask IRQ
    core_timer_enable();
    asm volatile("msr daifclr, #2");
}

void handle_core_irq() {
    timer_count++;
    system_time = timer_get_current_time();
    
    //uart_printf("Core Timer Interrupt! Count: %d\n", timer_count);
    while (timer_queue && timer_queue->expire_time <= system_time) {
        timer_event_t *event = timer_pop_event();
        if (event->callback) {
            event->callback(event->args);
        }
        free_timer_event(event);
    }
    
    // 設置下一個中斷時間
    if (timer_queue) {
        timer_set_next_interrupt(timer_queue->expire_time - system_time);
    } else {
        // 沒有事件也要重設中斷，保持 timer 正常運作
        core_timer_handler();
    }
}

uint64_t timer_get_current_time(void){
    uint64_t frequency = get_cntfrq_el0();
    uint64_t count = get_cntpct_el0();
    return (count * 1000) / frequency; // 返回毫秒
}

// 添加定時器事件到優先隊列（按到期時間排序）
// args 的內容會被複製到 event->data 中，callback 收到的是 event->data 的指標
void timer_add_event(void (*callback)(void*), void *args, uint64_t duration) {
    timer_event_t *new_event = alloc_timer_event();
    if (!new_event) {
        return; // 記憶體池已滿
    }
    
    // 將 args（字串）複製到 event 內部的 data buffer
    if (args) {
        const char *src = (const char *)args;
        int i;
        for (i = 0; i < TIMER_DATA_SIZE - 1 && src[i]; i++)
            new_event->data[i] = src[i];
        new_event->data[i] = '\0';
        new_event->args = new_event->data;  // 指向自己的 buffer
    } else {
        new_event->args = NULL;
    }
    
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
}

// 取消定時器事件
void timer_cancel_event(timer_event_t *event) {
    if (!timer_queue || !event) {
        return;
    }
    
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
                return;
            }
            current = current->next;
        }
    }
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

