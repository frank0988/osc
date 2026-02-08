#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stddef.h>

#define MAX_TIMER_EVENTS 32   // 最大定時器事件數量
#define TIMER_DATA_SIZE  64   // 事件內嵌資料大小

// Timer event structure
typedef struct timer_event {
    uint64_t expire_time;              // 到期時間
    void (*callback)(void*);           // 回調函數
    void *args;                        // 回調函數參數
    char data[TIMER_DATA_SIZE];        // 內嵌資料緩衝區
    struct timer_event *next;          // 鏈表下一個節點
    int in_use;                        // 是否正在使用
} timer_event_t;

void core_timer_enable(void);
void core_timer_handler(void);
uint64_t get_cntfrq_el0(void);
uint64_t get_cntpct_el0(void);
extern volatile int timer_count;

extern timer_event_t *timer_queue;      // 優先隊列頭
extern uint64_t system_time;

// Timer functions
void timer_init(void);
void handle_core_irq(void);
uint64_t timer_get_current_time(void);
void timer_add_event(void (*callback)(void*), void *args, uint64_t duration);
void timer_cancel_event(timer_event_t *event);
void timer_tick_handler(void);
timer_event_t* timer_pop_event(void);
void timer_set_next_interrupt(uint64_t duration);

#endif
