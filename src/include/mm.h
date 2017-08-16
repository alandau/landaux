#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <mem.h>
#include <list.h>
#include <fs.h>

#define MAP_USERSPACE	0x01
#define MAP_READWRITE	0x02

#define PROT_READ       0x1             /* Page can be read.  */
#define PROT_WRITE      0x2             /* Page can be written.  */
#define PROT_EXEC       0x4             /* Page can be executed.  */
#define PROT_NONE       0x0             /* Page can not be accessed.  */

/* Sharing types (must choose one and only one of these).  */
#define MAP_SHARED	0x01            /* Share changes.  */
#define MAP_PRIVATE	0x02            /* Changes are private.  */
#define MAP_TYPE	0x0f            /* Mask for type of mapping.  */

/* Other flags.  */
#define MAP_FIXED	0x10            /* Interpret addr exactly.  */
#define MAP_ANONYMOUS	0x20            /* Don't use a file.  */

typedef struct pte_struct		/* page table entry */
{
	u64 flags : 8;
	u64 reserved : 4;		/* bits 8-11, includes Intel-Reserved and Avail */
	u64 frame : 40;
	u64 avail : 11;
	u64 nx : 1;
} pte_t;

typedef struct page_struct {
	int count;
} page_t;

extern page_t *pages;

typedef struct vm_area_struct {
	u64 start;
	u64 end;
	int prot;
	file_t *file;
	u64 offset;
	list_t list;
} vm_area_t;

typedef struct mm_struct
{
	pte_t *page_dir;
	list_t vm_areas;
} mm_t;


void init_pmm(u64 base, u64 size);
void map_memory(u64 size);
void init_heap(void);




void init_mm(u64 size);

u64 alloc_phys_page(void);
u32 alloc_phys_pages(u32 count);
void free_phys_page(u64 frame);
void free_phys_pages(u32 frame, u32 count);

void *alloc_page(u32 flags);
void free_page(void *address);
void *alloc_pages(u32 count, u32 flags);
void free_pages(void *address, u32 count);

u64 get_mem_size(void);
u64 get_used_mem(void);
u64 get_free_mem(void);

void *ioremap(u32 phys_addr, u32 size, u32 virt_addr, u32 flags);
void iounmap(void *address, u32 size);
void map_kernel_page(u64 frame, u64 addr);
void map_user_page(u64 frame, u64 addr, u32 flags);

u32 virt_to_phys(void *address);

struct task_struct;
int clone_mm(struct task_struct *dest);
void free_mm(void);
u64 get_task_cr3(mm_t *mm);
void init_task_mm(mm_t *mm);

void *do_mmap(file_t *f, u64 offset, u64 vaddr, u64 len, int prot, int flags);

static inline void tlb_invalidate_entry(u64 address)
{
	__asm__ __volatile__ ("invlpg (%0)" : : "r"(address) : "memory");
}

static inline void tlb_invalidate_all(void)
{
	u64 tmp;
	__asm__ __volatile__ ("mov %%cr3, %0; mov %0, %%cr3" : "=r"(tmp): : "memory");
}

#endif
