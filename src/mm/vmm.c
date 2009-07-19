#include <kernel.h>
#include <string.h>
#include <mm.h>
#include <process.h>

pte_t page_dir[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t kernel_page_table[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));

void map_memory(u32 size)
{
	pte_t *pde = page_dir + (KERNEL_VIRT_ADDR >> 22);
	u32 i, j;
	int first_frame;
	/* The first 4MB are already mapped */
	size -= 4*1024*1024;
	first_frame = (4*1024*1024) / PAGE_SIZE;
	pde++;
	size >>= PAGE_SHIFT;
	for (i = 0; i < size / 1024; i++) {
		u32 frame = alloc_phys_page();
		pte_t *pte;
		BUG_ON(frame == 0);
		pde->frame = frame;
		pde->flags = PTE_PRESENT | PTE_WRITE;
		pde->reserved = 0;
		pte = (pte_t *)P2V(frame << PAGE_SHIFT);
		for (j = 0; j < 1024; j++) {
			pte->frame = first_frame + i * 1024 + j;
			pte->flags = PTE_PRESENT | PTE_WRITE;
			pte->reserved = 0;
			pte++;
		}
		pde++;
	}
	if ((size % 1024) != 0) {
		u32 frame = alloc_phys_page();
		pte_t *pte;
		BUG_ON(frame == 0);
		pde->frame = frame;
		pde->flags = PTE_PRESENT | PTE_WRITE;
		pde->reserved = 0;
		pte = (pte_t *)P2V(frame << PAGE_SHIFT);
		for (j = 0; j < size % 1024; j++) {
			pte->frame = first_frame + i * 1024 + j;
			pte->flags = PTE_PRESENT | PTE_WRITE;
			pte->reserved = 0;
			pte++;
		}

	}
	tlb_invalidate_all();
}

static void map_page(u32 frame, u32 addr, int flags)
{
	pte_t *pde, *pte;
	BUG_ON(addr & ~PAGE_MASK);
	addr >>= PAGE_SHIFT;
	pde = current->mm.page_dir + (addr>>10);
	if (!(pde->flags & PTE_PRESENT)) {
		u32 table = alloc_phys_page();
		BUG_ON(table == 0);
		memset((void *)P2V(table << PAGE_SHIFT), 0, PAGE_SIZE);
		pde->frame = table;
		pde->reserved = 0;
		pde->flags = PTE_PRESENT | PTE_WRITE | flags;
	}
	pte = (pte_t *)(P2V(pde->frame << PAGE_SHIFT)) + (addr & 0x3FF);
	BUG_ON(pte->flags & PTE_PRESENT);
	pte->frame = frame;
	pte->reserved = 0;
	pte->flags = PTE_PRESENT | flags;
	tlb_invalidate_entry(addr << PAGE_SHIFT);
}

void map_kernel_page(u32 frame, u32 addr)
{
	map_page(frame, addr, PTE_WRITE);
}

void map_user_page(u32 frame, u32 addr, u32 flags)
{
	map_page(frame, addr, PTE_USER | flags);
}

void *alloc_page(unsigned long flags)
{
	u32 frame = alloc_phys_page();
	if (!frame)
		return NULL;
	/* page is already mapped, since all memory is mapped */
	return (void *)P2V(frame << PAGE_SHIFT);
}

void free_page(void *p)
{
	BUG_ON((u32)p & ~PAGE_MASK);
	free_phys_page(V2P((u32)p) >> PAGE_SHIFT);
}

u32 get_task_cr3(mm_t *mm)
{
	return V2P((u32)mm->page_dir);
}
