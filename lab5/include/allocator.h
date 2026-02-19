#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "list.h"

#define PAGE_SIZE       4096
#define MAX_ORDER       18      
#define MEMORY_START    0x1F000000
#define MEMORY_END      0x2F000000 
#define PAGE_MASK (~(PAGE_SIZE - 1))

struct page {
    unsigned int isfree;
    int order;                  
    struct list_head lru;
};       

// 每個 Page Frame 的元數據
struct frame {
    struct list_head list;
    int order;
    int is_allocated;
    unsigned int index;
};

// 初始化接口
void mm_init();
void *page_alloc(int order);
void page_free(void *ptr);

// Dynamic Allocator 接口
void *kmalloc(unsigned int size);
void kfree(void *ptr);

void mm_zero(void *ptr, unsigned long size);

// 測試與調試
void dump_free_areas(void);
void mm_test(void);

#endif