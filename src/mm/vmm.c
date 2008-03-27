#include <mm.h>
#include <kernel.h>

pte_t page_dir[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t kernel_page_table[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));

#define MEMORY_MAP	0xE0000000

void map_memory(u32 size)
{
	pte_t *pde = page_dir + (MEMORY_MAP >> 22);
	u32 i, j;
	size >>= PAGE_SHIFT;
	BUG_ON((size % 1024) != 0);
	for (i = 0; i < size / 1024; i++) {
		u32 frame = alloc_phys_page();
		pte_t *pte;
		BUG_ON(frame == 0 || frame >= (4*1024*1024/PAGE_SIZE));	/* if frame is above 4MB we can't address it yet */
		pde->frame = frame;
		pde->flags = PTE_PRESENT | PTE_WRITE;
		pde->reserved = 0;
		pte = (pte_t *)((frame << PAGE_SHIFT) + KERNEL_VIRT_ADDR);
		for (j = 0; j < 1024; j++) {
			pte->frame = i * 1024 + j;
			pte->flags = PTE_PRESENT | PTE_WRITE;
			pte->reserved = 0;
			pte++;
		}
		pde++;
	}
}
