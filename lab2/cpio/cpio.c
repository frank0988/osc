#include "cpio.h"
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
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
typedef enum { CPIO_GET_HEADER, CPIO_GET_PATHNAME, CPIO_GET_DATA, GPIO_NEXT, CPIO_END } cpio_state_t;
typedef struct {
    const char  *current;
    cpio_state_t cs;
    unsigned int namesize;
    unsigned int filesize;
} cpio_info_t;
typedef void (*cpio_callback)(const char *name, const char *data, unsigned int size);
void cpio_for_each(void *addr, cpio_callback cb) {
    cpio_info_t info;
    info.current         = (const char *)addr;
    info.cs              = CPIO_GET_HEADER;
    const char *pathname = NULL;
    const char *data     = NULL;
    while (info.cs != CPIO_END) {
        cpio_state_t ns          = info.cs;
        const char  *header_base = info.current;

        switch (info.cs) {
            case CPIO_GET_HEADER:
                if (strncmp(info.current, "070701", 6) != 0) {
                    info.cs = CPIO_END;
                    break;
                }
                info.namesize = hex2int(info.current + 94, 8);
                info.filesize = hex2int(info.current + 54, 8);

                info.current += 110;
                ns = CPIO_GET_PATHNAME;
                break;

            case CPIO_GET_PATHNAME:
                pathname = info.current;

                if (strncmp(pathname, "TRAILER!!!", 10) == 0) {
                    ns = CPIO_END;
                    break;
                }

                unsigned int offset = (110 + info.namesize + 3) & ~3;
                info.current        = header_base + offset;

                ns = CPIO_GET_DATA;
                break;

            case CPIO_GET_DATA:
                data = info.current;

                if (cb) {
                    cb(pathname, data, info.filesize);
                }

                unsigned int data_offset = (info.filesize + 3) & ~3;
                info.current += data_offset;

                ns = CPIO_GET_HEADER;
                break;

            case CPIO_END:
            default:
                break;
        }
        info.cs = ns;
    }
}
