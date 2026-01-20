extern char           _end;  // 這是 Linker 指向 BSS 結尾的符號
static char          *heap_top = &_end;
typedef unsigned long size_t;

void *simple_malloc(size_t size) {
    heap_top = (char *)(((unsigned long)heap_top + 7) & ~7);

    void *allocated_ptr = heap_top;

    heap_top += size;

    return allocated_ptr;
}
