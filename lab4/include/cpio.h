#ifndef CPIO_H
#define CPIO_H
#define NULL ((void *)0)
void cpio_ls(void *addr);
void cpio_cat(void *addr, const char *filename);
const char* cpio_get_file(void *addr, const char *filename, unsigned int *out_size);
extern void *CPIO_DEFAULT_ADDR;
#endif