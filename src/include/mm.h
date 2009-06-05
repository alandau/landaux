#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <mem.h>
#include <list.h>

#define MAP_USERSPACE	0x01
#define MAP_READWRITE	0x02

#define VM_AREA_READ	0x04
#define VM_AREA_WRITE	0x02
#define VM_AREA_EXEC	0x01

typedef struct pte_struct		/* page table entry */
{
	u8 flags;
	u32 reserved : 4;		/* bits 8-12, includes Intel-Reserved and Avail */
	u32 frame : 20;
} pte_t;

typedef struct page_struct {
	int count;
} page_t;

extern page_t *pages;

typedef struct vm_area_struct {
	u32 start;
	u32 end;
	u32 flags;
	list_t list;
} vm_area_t;

typedef struct mm_struct
{
	pte_t *page_dir;
	list_t vm_areas;
} mm_t;


void init_pmm(u32 base, u32 size);
void map_memory(u32 size);
void init_heap(void);




void init_mm(u32 size);

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
void map_kernel_page(u32 frame, u32 addr);
void map_user_page(u32 frame, u32 addr, u32 flags);

u32 virt_to_phys(void *address);

struct task_struct;
int clone_mm(struct task_struct *dest);
void free_mm(mm_t *mm);
int mm_add_area(u32 start, u32 size, u32 flags);
u32 get_task_cr3(mm_t *mm);
void init_task_mm(mm_t *mm);

#endif
