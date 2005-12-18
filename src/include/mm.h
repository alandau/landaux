#ifndef MM_H
#define MM_H

#include <stddef.h>

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1<<PAGE_SHIFT)
#define PAGE_SIZE_K	(PAGE_SIZE/1024)

void init_mm(void);
void *alloc_page(void);
void free_page(void *address);
void *alloc_pages(u32 count);
void free_pages(void *address, u32 count);

u32 get_mem_size(void);
u32 get_used_mem(void);
u32 get_free_mem(void);

void *ioremap(u32 phys_addr, u32 size);
void iounmap(void *address, u32 size);

u32 virt_to_phys(void *address);

void *clone_mm(void *mm);

#endif
