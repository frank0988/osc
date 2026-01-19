#ifndef CPIO_H
#define CPIO_H
#define NULL ((void *)0)
void cpio_ls(void *addr);
void cpio_cat(void *addr, const char *filename);
extern void *CPIO_DEFAULT_ADDR;
#endif