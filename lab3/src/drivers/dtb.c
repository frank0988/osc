#include <stdint.h>
#include "dtb.h"
#include "uart.h"
extern void *dtb_ptr;
// Initial ram disk的起始位址，由 DTB Parser 解析 linux,initrd-start 後設定
void *initrd_start = NULL;
uint32_t uint32_be2le(const unsigned char *p) {
    //big-endian to little-endian
    return (uint32_t)p[0] << 24 |
           (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8  |
           (uint32_t)p[3];
}
uint64_t uint64_be2le(const unsigned char *p) {
    //big-endian to little-endian
    return (uint64_t)p[0] << 56 |
           (uint64_t)p[1] << 48 |
           (uint64_t)p[2] << 40 |
           (uint64_t)p[3] << 32 |
           (uint64_t)p[4] << 24 |
           (uint64_t)p[5] << 16 |
           (uint64_t)p[6] << 8  |
           (uint64_t)p[7];
}/*
void dtb_traverse(void *ptr, dtb_callback cb) {
    struct fdt_header *header = (struct fdt_header *)ptr;
    
    if (uint32_be2le((unsigned char *)&header->magic) != 0xd00dfeed) {
        return;
    }


    char *dt_struct_ptr = (char *)ptr + uint32_be2le((unsigned char *)&header->off_dt_struct);
    char *dt_strings_ptr = (char *)ptr + uint32_be2le((unsigned char *)&header->off_dt_strings);
    char *p = dt_struct_ptr;

    while (1) {
        uint32_t token = uint32_be2le((unsigned char *)p);
        p += 4; // skip token

        if (token == FDT_BEGIN_NODE) {
            char *node_name = p;
        
            cb(token, node_name, NULL, 0);
            
            // ! remember to align to 4-byte
            int len = 0;
            while(node_name[len] != '\0') len++;
            p = FDT_ALIGN(p + len + 1);

        } else if (token == FDT_PROP) {
            uint32_t data_size = uint32_be2le((unsigned char *)p);
            p += 4;
            uint32_t name_off = uint32_be2le((unsigned char *)p);
            p += 4;

            char *prop_name = dt_strings_ptr + name_off;
            cb(token, prop_name, p, data_size);

            // ! remember to align to 4-byte
            p = FDT_ALIGN(p + data_size);

        } else if (token == FDT_END_NODE || token == FDT_NOP) {
            cb(token, NULL, NULL, 0);
        } else if (token == FDT_END) {
            cb(token, NULL, NULL, 0);
            break;
        } else {
            break;
        }
    }
}
*/
void dtb_callback_find_initrd(uint32_t token, const char *name, const void *data, uint32_t size) {
    if (token == FDT_PROP && strcmp(name, "linux,initrd-start") == 0) {
        if (size == 8) {
            initrd_start = (void *)uint64_be2le(data);
        } else {
            initrd_start = (void *)(uintptr_t)uint32_be2le(data);
        }
    }
}
void debug_dump_callback(uint32_t token, const char *name, const void *data, uint32_t size) {
    uart_puts("DTB Token: ");
    uart_put_hex_32(token);
    if (token == FDT_BEGIN_NODE) {
        uart_puts("Node: ");
        uart_puts(name);
        uart_puts("\n");
    } else if (token == FDT_PROP) {
        uart_puts("  Prop: ");
        uart_puts(name);
        uart_puts("\n");
        
        // 如果是我們想要的屬性，順便檢查它的大小
        if (strcmp(name, "linux,initrd-start") == 0) {
            uart_puts("  [FOUND IT!] Size: ");
            uart_put_hex_32(size); // 看看是 4 還是 8
            uart_puts("\n");
        }
    }
}
void dtb_traverse(void *ptr, dtb_callback cb) {
    struct fdt_header *header = (struct fdt_header *)ptr;
    uint32_t *check = (uint32_t *)ptr;
    uart_puts("Memory Dump at DTB: ");
    uart_hex(check[0]); // 該印出 0xEDFE0DD0 (Big Endian)
    uart_puts(" ");
    uart_hex(check[1]);
    uart_puts("\n");
    
    // 1. 檢查 Magic Number
    uint32_t magic = uint32_be2le((unsigned char *)&header->magic);
    if (magic != 0xd00dfeed) {
        uart_puts("DTB Error: Invalid Magic Number 0x");
        uart_hex(magic);
        uart_puts("\n");
        return;
    }

    uart_puts("DTB: Magic verified. Starting traversal...\n");

    // 2. 取得結構區段位址
    char *dt_struct_ptr = (char *)ptr + uint32_be2le((unsigned char *)&header->off_dt_struct);
    char *dt_strings_ptr = (char *)ptr + uint32_be2le((unsigned char *)&header->off_dt_strings);
    char *p = dt_struct_ptr;

    while (1) {
        uint32_t token = uint32_be2le((unsigned char *)p);
        
        // 如果遇到 0，通常是 padding，跳過 4 bytes
        if (token == 0) {
            p += 4;
            continue;
        }

        p += 4; // 跳過 Token 本身

        if (token == FDT_BEGIN_NODE) {
            char *node_name = p;
            cb(token, node_name, NULL, 0);
            
            // 計算名稱長度並對齊
            int len = 0;
            while(node_name[len] != '\0') len++;
            p = (char *)FDT_ALIGN(p + len + 1);

        } else if (token == FDT_PROP) {
            uint32_t data_size = uint32_be2le((unsigned char *)p);
            p += 4;
            uint32_t name_off = uint32_be2le((unsigned char *)p);
            p += 4;

            char *prop_name = dt_strings_ptr + name_off;
            cb(token, prop_name, p, data_size);

            p = (char *)FDT_ALIGN(p + data_size);

        } else if (token == FDT_END_NODE || token == FDT_NOP) {
            cb(token, NULL, NULL, 0);
            // token 已經在前面 p += 4 跳過了，這裡不需要額外操作
        } else if (token == FDT_END) {
            cb(token, NULL, NULL, 0);
            uart_puts("DTB: Traversal finished normally.\n");
            break;
        } else {
            uart_puts("DTB Error: Unknown Token 0x");
            uart_hex(token);
            uart_puts(" at 0x");
            uart_hex((uint64_t)p-4);
            uart_puts("\n");
            break;
        }
    }
}