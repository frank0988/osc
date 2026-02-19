#ifndef THREAD_H
#define THREAD_H
#define THREAD_STACK_SIZE 4096
#define USER_STACK_SIZE  4096

// Trap Frame: 對應 save_all 在 kernel stack 上的佈局
// 當 exception 從 EL0 或 EL1 進入時，所有暫存器保存在此結構
//
// Assembly layout (save_all):
//   [sp, 16*0 .. 16*14]: x0-x29 (stp pairs)
//   [sp, 16*15]:         x30    (str, 只有 8 bytes)
//   [sp, 16*15+8]:       --- gap ---
//   [sp, 16*16]:         spsr_el1 (stp low)
//   [sp, 16*16+8]:       elr_el1  (stp high)
//   [sp, 16*17]:         sp_el0   (str)
//   [sp, 16*17+8]:       --- padding ---
struct trap_frame {
    unsigned long regs[31];   // x0-x30 (offset 0-247, 31×8=248 bytes)
    unsigned long __pad0;     // offset 248 (gap: x30 只佔 8 bytes, 但 stp 對齊到 16*16)
    unsigned long spsr_el1;   // offset 256 (= 16*16)
    unsigned long elr_el1;    // offset 264 (= 16*16 + 8)
    unsigned long sp_el0;     // offset 272 (= 16*17)
    unsigned long __pad1;     // offset 280 (alignment padding)
};
enum thread_status {
    THREAD_FREE,
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_WAITING, // 未來 Lab 會用到
    THREAD_DEAD     // Zombie，等待回收
};

// 定義 Context (對應 switch_to 的儲存內容)
// 注意：根據 ARM64 ABI，我們需要保存 x19-x28, fp, lr, sp
struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp; // x29
    unsigned long lr; // x30
    unsigned long sp; 
};

struct thread {
    struct cpu_context context; // 必須放在第一個，因為 switch_to 預設存取 offset 0
    long id;                    // Thread ID
    enum thread_status status;  // 狀態
    int preempt_count;          // > 0 時禁止搶佔 (preemption)
    char *stack;                // kernel 堆疊記憶體指標 (用於釋放)
    char *user_stack;           // user 堆疊 (NULL for kernel threads)
    struct thread *next;        // 用於 Run Queue (Linked List)
    struct thread *prev;        // 用於 Run Queue
};

// 全域變數
extern struct thread *run_queue;
extern struct thread *current_thread;
extern struct thread *idle_thread_ptr;
extern long generate_new_pid(void);

// Preemption 控制
void preempt_disable(void);
void preempt_enable(void);

// Thread 管理函式
struct thread *thread_create(void (*func)(void));
struct thread *thread_create_user(void (*func)(void));
void thread_exit(void);

// 排程函式
void schedule(void);
void add_to_runqueue(struct thread *t);
void remove_from_runqueue(struct thread *t);
struct thread *get_current(void);

// Scheduler 初始化和啟動
void scheduler_init(void);
void scheduler_start(void);

// Zombie 回收
void kill_zombies(void);

// 檢查是否還有 user threads 在執行
int has_user_threads(void);

// Context Switch (assembly)
void switch_to(struct cpu_context *prev, struct cpu_context *next);

// EL1 → EL0 切換 (assembly, in kernel_start.S)
void from_el1_to_el0(unsigned long entry, unsigned long user_sp);

#endif