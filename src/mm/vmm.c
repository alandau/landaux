#include <mm.h>
#include <kernel.h>
#include <string.h>

pte_t page_dir[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t kernel_page_table[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));

void map_memory(u32 size)
{
	pte_t *pde = page_dir + (KERNEL_VIRT_ADDR >> 22);
	u32 i, j;
	/* The first 4MB are already mapped */
	size -= 4*1024*1024;
	pde++;
	size >>= PAGE_SHIFT;
	BUG_ON((size % 1024) != 0);
	for (i = 0; i < size / 1024; i++) {
		u32 frame = alloc_phys_page();
		pte_t *pte;
		BUG_ON(frame == 0);
		pde->frame = frame;
		pde->flags = PTE_PRESENT | PTE_WRITE;
		pde->reserved = 0;
		pte = (pte_t *)P2V(frame << PAGE_SHIFT);
		for (j = 0; j < 1024; j++) {
			pte->frame = i * 1024 + j;
			pte->flags = PTE_PRESENT | PTE_WRITE;
			pte->reserved = 0;
			pte++;
		}
		pde++;
	}
	__asm__ __volatile__ ("movl %%cr3, %%eax; movl %%eax, %%cr3" : : : "eax", "memory");
}

void map_page(u32 frame, u32 addr)
{
	pte_t *pde, *pte;
	BUG_ON(addr & ~PAGE_MASK);
	addr >>= PAGE_SHIFT;
	pde = page_dir + (addr>>10);
	if (!(pde->flags & PTE_PRESENT)) {
		u32 table = alloc_phys_page();
		BUG_ON(table == 0);
		memset((void *)P2V(table << PAGE_SHIFT), 0, PAGE_SIZE);
		pde->frame = table;
		pde->reserved = 0;
		pde->flags = PTE_PRESENT | PTE_WRITE;
	}
	pte = (pte_t *)(P2V(pde->frame << PAGE_SHIFT)) + (addr & 0x3FF);
	BUG_ON(pte->flags & PTE_PRESENT);
	pte->frame = frame;
	pte->reserved = 0;
	pte->flags = PTE_PRESENT | PTE_WRITE;
	__asm__ __volatile__ ("invlpg (%0)" : : "r"(addr << PAGE_SHIFT) : "memory");
}
