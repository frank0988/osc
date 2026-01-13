#include "reset.h"
extern void         put32(unsigned long addr, unsigned int val);
extern unsigned int get32(unsigned long addr);

void reset(int tick) {
    put32(PM_RSTC, PM_PASSWORD | 0x20);
    put32(PM_WDOG, PM_PASSWORD | tick);
}
