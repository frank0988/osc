#include <stdint.h>
uint32_t big_endian_to_small(void *p) {
    unsigned char *begin = (unsigned char *)p;
    uint32_t       res   = 0;

    res |= (uint32_t)begin[0] << 24;
    res |= (uint32_t)begin[1] << 16;
    res |= (uint32_t)begin[2] << 8;
    res |= (uint32_t)begin[3] << 0;

    return res;
}
