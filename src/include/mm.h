#ifndef MM_H
#define MM_H

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1<<PAGE_SHIFT)
#define PAGE_SIZE_K	(PAGE_SIZE/1024)

void init_mm(void);
void *alloc_page(void);
void free_page(void *address);
void *alloc_pages(unsigned long count);
void free_pages(void *address, unsigned long count);

unsigned long get_mem_size(void);
unsigned long get_used_mem(void);
unsigned long get_free_mem(void);

void *ioremap(unsigned long phys_addr, unsigned long size);
void iounmap(void *address, unsigned long size);

#endif
