#include <kernel.h>
#include <string.h>
#include <mm.h>
#include <process.h>

pte_t page_table_l4[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t page_table_l3[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t page_table_l2[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t page_table_l3_memmap[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));

void map_memory(u64 size) {
	BUG_ON(size > 1024*1024*1024);
	pte_t *l4e = &page_table_l4[(MEMORY_BASE >> 39) & 0x1ff];
	l4e->flags = PTE_PRESENT | PTE_WRITE;
	l4e->frame = (u64)V2P_IMAGE((u64)page_table_l3_memmap) >> 12;
	pte_t *l3e = &page_table_l3_memmap[(MEMORY_BASE >> 30) & 0x1ff];
	l3e->flags = PTE_PRESENT | PTE_WRITE | PTE_SIZE;
	l3e->frame = 0;
	tlb_invalidate_all();
}

static void map_page(u64 frame, u64 addr, int flags)
{
	BUG_ON(addr & ~PAGE_MASK);
	addr >>= PAGE_SHIFT;
	pte_t *table = current->mm.page_dir;
	for (int i = 3; i >= 1; i--) {
		pte_t *pte = table + ((addr >> i*9) & 0x1ff);	
		if (pte->flags & PTE_PRESENT) {
			table = P2V(pte->frame << PAGE_SHIFT);
			continue;
		}
		u64 new_table = alloc_phys_page();
		BUG_ON(new_table == 0);
		table = P2V(new_table << PAGE_SHIFT);
		memset(table, 0, PAGE_SIZE);
		memset(pte, 0, sizeof(pte_t));
		pte->frame = new_table;
		pte->flags = PTE_PRESENT | PTE_WRITE | flags;
	}
	pte_t *pte = table + (addr & 0x1ff);
	BUG_ON(pte->flags & PTE_PRESENT);
	memset(pte, 0, sizeof(pte_t));
	pte->frame = frame;
	pte->flags = PTE_PRESENT | flags;
	tlb_invalidate_entry(addr << PAGE_SHIFT);
}

void map_kernel_page(u64 frame, u64 addr)
{
	map_page(frame, addr, PTE_WRITE);
}

void map_user_page(u64 frame, u64 addr, u32 flags)
{
	map_page(frame, addr, PTE_USER | flags);
}

void *alloc_page(u32 flags)
{
	u64 frame = alloc_phys_page();
	if (!frame)
		return NULL;
	/* page is already mapped, since all memory is mapped */
	return (void *)P2V(frame << PAGE_SHIFT);
}

void free_page(void *p)
{
	BUG_ON((u64)p & ~PAGE_MASK);
	free_phys_page(V2P((u64)p) >> PAGE_SHIFT);
}

u64 get_task_cr3(mm_t *mm)
{
	return V2P((u64)mm->page_dir);
}
