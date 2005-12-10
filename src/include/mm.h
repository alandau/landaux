#ifndef MM_H
#define MM_H

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1<<PAGE_SHIFT)
#define PAGE_SIZE_K	(PAGE_SIZE/1024)

void init_mm(void);
void *alloc_page(void);
void free_page(void *address);
unsigned long get_mem_size();
unsigned long get_used_mem();
unsigned long get_free_mem();

#endif
