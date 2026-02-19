#include "allocator.h"
#include "uart.h"
extern char           _end;  // 這是 Linker 指向 BSS 結尾的符號
extern char           _stack_top; // Linker 定義的 stack 頂端
static char          *heap_top = 0;  // 運行時初始化
typedef unsigned long size_t;

struct list_head free_areas[MAX_ORDER];
struct page *mem_map;
void *simple_malloc(size_t size) {
    // 第一次調用時初始化 heap_top（從 stack 之後開始，避免覆蓋 stack）
    if (heap_top == 0) {
        heap_top = &_stack_top;
    }
    heap_top = (char *)(((unsigned long)heap_top + 7) & ~7);

    void *allocated_ptr = heap_top;

    heap_top += size;

    return allocated_ptr;
}
void mm_init() {
    
    // 初始化所有 free_areas
    for (int i = 0; i < MAX_ORDER; i++) {
        INIT_LIST_HEAD(&free_areas[i]);
    }
    
    // 計算總頁數
    unsigned long total_pages = (MEMORY_END - MEMORY_START) / PAGE_SIZE;
    
    // 分配 mem_map 陣列
    mem_map = (struct page *)simple_malloc(total_pages * sizeof(struct page));
    
    // 初始化所有 page 結構
    for (unsigned long i = 0; i < total_pages; i++) {
        INIT_LIST_HEAD(&mem_map[i].lru);
        mem_map[i].order = 0;
        mem_map[i].isfree = 0; 
    }
    
    // 計算可用記憶體的起始位置
    unsigned long free_start = ((unsigned long)heap_top + PAGE_SIZE - 1) & PAGE_MASK;
    
    // 確保 free_start >= MEMORY_START
    if (free_start < MEMORY_START) {
        free_start = MEMORY_START;
    }
    
    unsigned long free_end = MEMORY_END & PAGE_MASK;
    
    // 將可用的實體位址逐頁釋放，觸發 buddy 合併
    for (unsigned long addr = free_start; addr < free_end; addr += PAGE_SIZE) {
        page_free((void *)addr);
    }
    
    uart_puts("[mm_init] Memory initialization complete!\n");
    uart_puts("\n[mm_init] Checking Initialization Result...\n");
    
    // 1. 輸出計算出的記憶體範圍，確認沒有計算錯誤
    uart_printf("  Mem Start : 0x%x\n", MEMORY_START);
    uart_printf("  Heap Top  : 0x%x (End of mem_map)\n", (unsigned long)heap_top);
    uart_printf("  Free Start: 0x%x (Aligned)\n", free_start);
    uart_printf("  Free End  : 0x%x\n", free_end);
    uart_printf("  Total Physical Pages: %d\n", total_pages);
    uart_printf ("mm_init_end1");
    // 2. 驗證合併效果 (最重要)
    // 如果合併成功，你不應該看到大量的 Order 0 或 Order 1。
    // 你應該看到大部分記憶體集中在 Order (MAX_ORDER-1) 或接近最大值的 Block 中。
    dump_free_areas();
}


void *page_alloc(int request_order) {
    if (request_order < 0 || request_order >= MAX_ORDER) {
        return (void *)0;
    }
    
    // 從 request_order 開始找可用的 block
    for (int order = request_order; order < MAX_ORDER; order++) {
        if (list_empty(&free_areas[order])) {
            continue;
        }
        
        // 找到可用的 block，從 free list 取出
        struct list_head *entry = free_areas[order].next;
        struct page *page = container_of(entry, struct page, lru);
        list_del(entry);
        
        page->isfree = 0;
        
        // 如果 block 太大，需要拆分
        while (order > request_order) {
            order--;
            
            // 計算 buddy 的 pfn（後半部分）
            unsigned long pfn = page - mem_map;
            unsigned long buddy_pfn = pfn + (1UL << order);
            struct page *buddy = &mem_map[buddy_pfn];
            
            // 將 buddy（後半部分）加入較小 order 的 free list
            buddy->order = order;
            buddy->isfree = 1;
            list_add(&buddy->lru, &free_areas[order]);
        }
        
        page->order = request_order;
        
        // 計算並返回實體地址
        unsigned long pfn = page - mem_map;
        return (void *)(MEMORY_START + pfn * PAGE_SIZE);
    }
    
    // 沒有足夠大的 block
    return (void *)0;
}

void page_free(void *ptr) {
    unsigned long addr = (unsigned long)ptr;
    
    // 確保地址對齊到 PAGE_SIZE
    addr = addr & PAGE_MASK;
    
    // 地址範圍檢查
    if (addr < MEMORY_START || addr >= MEMORY_END) {
        return;
    }
    
    // 計算 page index
    unsigned long pfn = (addr - MEMORY_START) / PAGE_SIZE;
    struct page *page = &mem_map[pfn];
    
    // 標記為 free，初始 order 為 0
    int order = page->order; 
    
    page->isfree = 1;
    
    // 嘗試與 buddy 合併，直到無法合併或達到 MAX_ORDER-1
    while (order < MAX_ORDER - 1) {
        // 用 XOR 計算 buddy 的 index
        unsigned long buddy_pfn = pfn ^ (1UL << order);
        
        // 檢查 buddy 是否在有效範圍內
        unsigned long total_pages = (MEMORY_END - MEMORY_START) / PAGE_SIZE;
        if (buddy_pfn >= total_pages) {
            break;
        }
        
        struct page *buddy = &mem_map[buddy_pfn];
        
        // Buddy 必須是 free 且 order 相同才能合併
        if (!buddy->isfree || buddy->order != order) {
            break;
        }
        
        // 從 free list 中移除 buddy
        list_del(&buddy->lru);
        buddy->isfree = 0;  // buddy 不再是獨立的 free block
        
        // 合併後取較小的 pfn 作為新 block 的起始
        if (buddy_pfn < pfn) {
            pfn = buddy_pfn;
            page = buddy;
        }
        
        order++;
        page->order = order;
        page->isfree = 1;
    }
    
    // 將合併後的 block 加入對應 order 的 free list
    list_add(&page->lru, &free_areas[order]);
}
// 計算 size 需要的 order
static int size_to_order(unsigned int size) {
    if (size == 0) return 0;
    
    // 至少需要 1 個 page
    unsigned int pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    int order = 0;
    unsigned int p = 1;
    while (p < pages) {
        p <<= 1;
        order++;
    }
    return order;
}

void *kmalloc(unsigned int size) {
    if (size == 0) {
        return (void *)0;
    }
    
    int order = size_to_order(size);
    if (order >= MAX_ORDER) {
        return (void *)0;
    }
    
    return page_alloc(order);
}
void kfree(void *ptr) {
    if (ptr == (void *)0) {
        return;
    }
    page_free(ptr);
}

// ========== 測試與調試函數 ==========

// 顯示 free_areas 的狀態
void dump_free_areas(void) {
    uart_printf("\n=== Buddy Allocator Status ===\n");
    uart_printf("Order | Block Size | Free Blocks\n");
    uart_printf("------+------------+------------\n");
    
    unsigned long total_free_pages = 0;
    
    for (int order = 0; order < MAX_ORDER; order++) {
        int count = 0;
        struct list_head *pos;
        
        // 計算這個 order 有多少個 free block
        for (pos = free_areas[order].next; pos != &free_areas[order]; pos = pos->next) {
            count++;
        }
        
        if (count > 0) {
            unsigned long block_size = PAGE_SIZE * (1UL << order);
            uart_printf("  %d   |  %d KB  |     %d\n", 
                       order, (int)(block_size / 1024), count);
            total_free_pages += count * (1UL << order);
        }
    }
    
    uart_printf("------+------------+------------\n");
    uart_printf("Total free memory: %d KB (%d pages)\n", 
               (int)(total_free_pages * PAGE_SIZE / 1024), (int)total_free_pages);
    uart_printf("================================\n\n");
}

// 分配測試並顯示結果
void mm_test(void) {
    uart_printf("\n========== Memory Allocator Test ==========\n");
    
    // 測試前狀態
    uart_printf("[Initial State]\n");
    dump_free_areas();
    
    // 測試 1: 基本分配
    uart_printf("[Test 1] Allocating 1 page (order 0)...\n");
    void *p1 = page_alloc(0);
    uart_printf("  Allocated at: 0x%x\n", (unsigned int)(unsigned long)p1);
    dump_free_areas();
    
    // 測試 2: 分配較大的 block
    uart_printf("[Test 2] Allocating 4 pages (order 2)...\n");
    void *p2 = page_alloc(2);
    uart_printf("  Allocated at: 0x%x\n", (unsigned int)(unsigned long)p2);
    dump_free_areas();
    
    // 測試 3: 再分配一個
    uart_printf("[Test 3] Allocating 2 pages (order 1)...\n");
    void *p3 = page_alloc(1);
    uart_printf("  Allocated at: 0x%x\n", (unsigned int)(unsigned long)p3);
    dump_free_areas();
    
    // 測試 4: 釋放 p1，觀察是否回到 order 0
    uart_printf("[Test 4] Freeing p1 (1 page)...\n");
    page_free(p1);
    dump_free_areas();
    
    // 測試 5: 釋放 p3，觀察 buddy 合併
    uart_printf("[Test 5] Freeing p3 (2 pages)...\n");
    page_free(p3);
    uart_printf("  (Should trigger buddy merge if p1 and p3 are buddies)\n");
    dump_free_areas();
    
    // 測試 6: 釋放 p2
    uart_printf("[Test 6] Freeing p2 (4 pages)...\n");
    page_free(p2);
    uart_printf("  (Should trigger more buddy merges)\n");
    dump_free_areas();
    
    // 測試 7: kmalloc 測試
    uart_printf("[Test 7] kmalloc(5000) - needs 2 pages...\n");
    void *p4 = kmalloc(5000);
    uart_printf("  Allocated at: 0x%x\n", (unsigned int)(unsigned long)p4);
    dump_free_areas();
    
    // 測試 8: kfree
    uart_printf("[Test 8] kfree(p4)...\n");
    kfree(p4);
    dump_free_areas();
    
    uart_printf("========== Test Complete ==========\n\n");
}




