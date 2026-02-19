#ifndef DTB_H  
#define DTB_H

#include <stdint.h>
#include <stddef.h>
#define FDT_ALIGN(addr) (((uintptr_t)(addr) + 3) & ~3)
typedef enum {
    FDT_BEGIN_NODE = 0x00000001,
    FDT_END_NODE   = 0x00000002,
    FDT_PROP       = 0x00000003,
    FDT_NOP        = 0x00000004,
    FDT_END        = 0x00000009
} fdt_token;


struct fdt_header {
    uint32_t magic;             // 0xd00dfeed
    uint32_t totalsize;
    uint32_t off_dt_struct;    
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

typedef void (*dtb_callback)(uint32_t token, const char *name, const void *data, uint32_t size);
extern void *initrd_start;
void dtb_traverse(void *ptr, dtb_callback cb);
void dtb_callback_find_initrd(uint32_t token, const char *name, const void *data, uint32_t size);
void debug_dump_callback(uint32_t token, const char *name, const void *data, uint32_t size);
uint32_t uint32_be2le(const unsigned char *p);
uint64_t uint64_be2le(const unsigned char *p);
#endif