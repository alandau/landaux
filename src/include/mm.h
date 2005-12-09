#ifndef MM_H
#define MM_H

#define PAGE_SIZE_K	4
#define PAGE_SIZE	(PAGE_SIZE_K*1024)

void init_mm(void);
void *alloc_page(void);
void free_page(void *address);

#endif
