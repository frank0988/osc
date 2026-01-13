#include "cpio.h"
#include "uart.h"
typedef unsigned long size_t;
unsigned int          hex2int(const char *s, int len) {
    unsigned int r = 0;
    for (int i = 0; i < len; i++) {
        r *= 16;
        if (s[i] >= '0' && s[i] <= '9') {
            r += s[i] - '0';
        } else if (s[i] >= 'a' && s[i] <= 'f') {
            r += s[i] - 'a' + 10;
        } else if (s[i] >= 'A' && s[i] <= 'F') {
            r += s[i] - 'A' + 10;
        }
    }
    return r;
}

typedef enum { CPIO_GET_HEADER, CPIO_GET_PATHNAME, CPIO_GET_DATA, GPIO_NEXT, CPIO_END } cpio_state_t;
typedef struct {
    const char  *current;
    cpio_state_t cs;
    unsigned int namesize;
    unsigned int filesize;
} cpio_info_t;
typedef void (*cpio_callback)(const char *name, const char *data, unsigned int size, void *arg);
void cpio_for_each(void *addr, cpio_callback cb, void *arg) {
    const char *current = (const char *)addr;

    while (1) {
        if (strncmp(current, "070701", 6) != 0) break;

        unsigned int namesize = hex2int(current + 94, 8);
        unsigned int filesize = hex2int(current + 54, 8);

        const char *pathname = current + 110;

        if (strncmp(pathname, "TRAILER!!!", 10) == 0) break;

        unsigned int data_offset = (110 + namesize + 3) & ~3;
        const char  *data        = current + data_offset;

        if (cb) {
            cb(pathname, data, filesize, arg);
        }

        unsigned int next_offset = (data_offset + filesize + 3) & ~3;
        current += next_offset;
    }
}
void ls_callback(const char *name, const char *data, unsigned int size, void *arg) {
    uart_puts(name);
    uart_send('\n');
}
void cpio_ls(void *addr) { cpio_for_each(addr, ls_callback, 0); }
typedef struct {
    const char  *target;  // 要找的檔名 (輸入)
    const char  *data;    // 找到的內容位址 (輸出)
    unsigned int size;    // 找到的內容大小 (輸出)
} find_ctx;

void find_callback(const char *name, const char *data, unsigned int size, void *arg) {
    find_ctx *ctx = (find_ctx *)arg;

    // 如果已經找到了，就不再比對
    if (ctx->data) return;

    // 比對檔名 (假設你有實作 strcmp)
    if (strcmp(name, ctx->target) == 0) {
        ctx->data = data;
        ctx->size = size;
    }
}

void cpio_cat(void *addr, const char *filename) {
    find_ctx ctx = {.target = filename, .data = 0, .size = 0};

    cpio_for_each(addr, find_callback, &ctx);

    if (ctx.data) {
        // 印出檔案內容 (注意檔案可能不是以 \0 結尾，需根據 size 印出)
        for (unsigned int i = 0; i < ctx.size; i++) {
            uart_send(ctx.data[i]);
        }
        uart_puts("\n");
    } else {
        uart_puts("File not found.\n");
    }
}
