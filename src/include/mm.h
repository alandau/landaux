#ifndef MM_H
#define MM_H

#include <stddef.h>

#define PAGE_SHIFT				12
#define PAGE_SIZE				(1<<PAGE_SHIFT)
#define PAGE_SIZE_K				(PAGE_SIZE/1024)
#define KERNEL_VIRT_ADDRESS		0xC0000000

#define MAP_USERSPACE	0x01
#define MAP_READWRITE	0x02

typedef struct pte_struct		/* page table entry */
{
	u8 flags;
	u32 reserved : 4;		/* bits 8-12, includes Intel-Reserved and Avail */
	u32 frame : 20;
} pte_t;

typedef struct mm_struct
{
	pte_t *page_dir;
} mm_t;

void init_mm(void);

u32 alloc_phys_page(void);
u32 alloc_phys_pages(u32 count);
void free_phys_page(u32 frame);
void free_phys_pages(u32 frame, u32 count);

void *alloc_page(u32 flags);
void free_page(void *address);
void *alloc_pages(u32 count, u32 flags);
void free_pages(void *address, u32 count);

u32 get_mem_size(void);
u32 get_used_mem(void);
u32 get_free_mem(void);

void *ioremap(u32 phys_addr, u32 size, u32 virt_addr, u32 flags);
void iounmap(void *address, u32 size);

u32 virt_to_phys(void *address);

int clone_mm(mm_t *dest_mm);
void free_mm(mm_t *mm);
u32 get_task_cr3(mm_t *mm);
void init_task_mm(mm_t *mm);

#endif
