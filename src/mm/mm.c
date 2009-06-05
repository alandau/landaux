#include <mm.h>
#include <arch.h>
#include <string.h>
#include <kernel.h>
#include <process.h>

#define CMOS_REG		0x70
#define CMOS_DATA		0x71

#define ONE_MB			0x100000


#define PHYS_MEM				0xF0000000
#define MAX_MEM_SIZE			(256*1024*1024)	/* used to calculate bitmap size */

#if 0
static u32 memsize;		/* in bytes */
static u32 memused;		/* in bytes */
static u32 bitmap[MAX_MEM_SIZE / PAGE_SIZE / 32];
static u32 bitmap_size;				/* number of unsigned long's in mem_bitmap */

pte_t first_mb_table[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
#endif

#if 0
/* in bytes */
static u32 __get_memsize(void)
{
	u32 total;
	u8 tmp;
	outb(CMOS_REG, 0x18);
	tmp = inb(CMOS_DATA);
	total = tmp << 8;
	outb(CMOS_REG, 0x17);
	tmp = inb(CMOS_DATA);
	total += tmp;
	return (total + 1024) * 1024;
}
#endif

#if 0
/* returns frame number of allocated frame */
u32 alloc_phys_page(void)
{
	int i, bit = 1, bit_offs = 0;
	for (i = 0; i < bitmap_size; i++)
	{
		if (bitmap[i] != 0xFFFFFFFF) break;
	}
	if (i == bitmap_size) return 0;	/* no free memory */
	/* not an endless loop, since free space exists */
	while (bitmap[i] & bit)
	{
		bit <<= 1;
		bit_offs++;
	}
	bitmap[i] |= bit;			/* mark it as used */
	memused += PAGE_SIZE;
	return i*32 + bit_offs;		/* 32 == bits in long */
}
#endif
#if 0
static int have_enough_zeros(u32 *p, u32 start_bit, u32 max_bits, int zeros)
{
	for (; zeros && max_bits; max_bits--, zeros--)
	{
		if (p[start_bit/32] & (1<<(start_bit%32))) return 0;
		start_bit++;
	}
	return (zeros == 0);
}

// allocates count consecutive frames
// returns the frame number of the first page
u32 alloc_phys_pages(u32 count)
{
	int i, bit;
	if (count == 0) return 0;
	for (i=0; i < bitmap_size; i++)
	{
		if (bitmap[i] == 0xFFFFFFFF) continue;
		for (bit = 0; bit < 32; bit++)
		{
			if (!have_enough_zeros(bitmap, i*32 + bit, bitmap_size*32, count)) continue;
			u32 pos = i*32;
			u32 ret = pos + bit;
			memused += (count << PAGE_SHIFT);
			while (count--)
			{
				bitmap[pos] |= (1<<bit);
				bit++;
				if (bit == 32)
				{
					bit = 0;
					pos++;
				}
			}
			return ret;
		}
	}
	return 0;
}

void free_phys_pages(u32 frame, u32 count)
{
	if (count == 0) return;
	if (frame/32 >= bitmap_size) BUG();
	memused -= (count << PAGE_SHIFT);
	while (count--)
	{
		if (!(bitmap[frame/32] & (1<<(frame%32)))) BUG();
		bitmap[frame/32] &= ~(1<<(frame%32));
		frame++;
	}
}

/* returns frame number of the zeroed allocated frame */
static u32 alloc_zeroed_phys_page(void)
{
	u32 frame = alloc_phys_page();
	if (!frame) return 0;
	memset((void *)(PHYS_MEM + frame*PAGE_SIZE), 0, PAGE_SIZE);
	return frame;
}
#endif
#if 0
/* frees frame by number */
void free_phys_page(u32 frame)
{
	int bit_offs, bit, i;
	i = frame / 32;			/* 4 == sizeof(unsigned long) */
	if (i >= bitmap_size) BUG();
	bit_offs = frame % 32;
	bit = 1 << bit_offs;
	if ((bitmap[i] & bit) == 0)		/* page is free */
		BUG();
	bitmap[i] &= ~bit;
	memused -= PAGE_SIZE;
}
#endif
#if 0
/* allocates a pagetable page to be pointed by first
   returns the allocated frame number */
static u32 alloc_page_table_page(pte_t *first)
{
	if (first->flags & PTE_PRESENT) BUG();
	u32 table = alloc_zeroed_phys_page();
	if (!table) BUG();
	first->flags = PTE_PRESENT | PTE_WRITE;
	first->frame = table;
	return table;
}
#endif
#if 0
void init_mm(void)
{
	u32 i, kernel_pages;
	memsize = __get_memsize();
	memused = 0;
	extern unsigned char _code[], _end[];
	u32 kernel_size = _end-_code;
	/* Assume memory size is a multiple of 128K */
	bitmap_size = (memsize / PAGE_SIZE) / 32;
	/* bitmap is in BSS and thus already zeroed */

	/* mark that the memory used by kernel: start at 0x1000=4KB, end at '_end'
	   mark 1st page (0x0-0x1000) as used too */
	bitmap[0] |= 1;
	kernel_pages = kernel_size / PAGE_SIZE;
	if (kernel_size % PAGE_SIZE) kernel_pages++;
	for (i=1; i<1+kernel_pages; i++)
		bitmap[i/32] |= 1 << (i%32);
	memused += (1+kernel_pages) * PAGE_SIZE;

	for (i=0xA0000/PAGE_SIZE; i<ONE_MB/PAGE_SIZE; i++)
		bitmap[i/32] |= 1 << (i%32);
	memused += ONE_MB-0xA0000;

	// map all physical memory at virtual address PHYS_MEM
	for (i=0; i<memsize/PAGE_SIZE; i++)
	{
		pte_t *first = &page_dir[(PHYS_MEM/PAGE_SIZE+i)/1024];
		pte_t *second;
		if (!(first->flags & PTE_PRESENT))
		{
			// allocate second-level page table
			if (first->flags & PTE_PRESENT) BUG();
			u32 table = alloc_phys_page();
			if (!table || (table<<PAGE_SHIFT) >= ONE_MB) BUG();
			first->flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
			first->frame = table;
			// zero it
			memset((void *)(table<<PAGE_SHIFT), 0, PAGE_SIZE);
		}
		// first->frame < 1MB, thus phys_addr=virt_addr
		second = (pte_t *)(first->frame*PAGE_SIZE) + (i%1024);
		second->flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
		second->frame = i;
	}

	// mark all pages in the range 0-3GB accessible from usermode
	for (i=0; i<KERNEL_VIRT_ADDR/(4*1024*1024); i++)
		page_dir[i].flags |= PTE_USER;

	// unmap the first mb
	asm ("nop;nop;nop;nop;nop");
//	page_dir[0].flags &= ~PTE_PRESENT;
//	int i = ((unsigned long)first_mb_table - KERNEL_VIRT_ADDR) >> PAGE_SHIFT;
//	bitmap[i/32] &= ~(1 << (i%32));
	// for now, unmap first page, to catch NULL dereferences
	pte_t *second = (pte_t *)(PHYS_MEM+page_dir[0].frame*PAGE_SIZE);
	second->flags &= ~PTE_PRESENT;

	init_task_mm(&current->mm);
}

u32 get_mem_size()
{
	return memsize;
}

u32 get_used_mem()
{
	return memused;
}

u32 get_free_mem()
{
	return memsize - memused;
}
#endif

#if 0
// given the pte at (first1, second1), find the next pte and put it's address at (*first2, *second2)
// *second2 will be NULL if *first2 points to a not-yet-allocated page
// returns 0 if *first2 went out of page_dir, 1 otherwise
static int get_next_pte(pte_t *first1, pte_t *second1, pte_t **first2, pte_t **second2, mm_t *mm)
{
	second1++;
	if (!((u32)second1 & (PAGE_SIZE-1)))		// increment first
	{
		*first2 = first1+1;
		*second2 = NULL;
		if (*first2 - mm->page_dir >= PAGE_SIZE/sizeof(pte_t)) return 0;
		if ((*first2)->flags & PTE_PRESENT)				// *first2 is allocated, set *second2 to the first entry in it
		{
			*second2 = (pte_t *)(PHYS_MEM + ((*first2)->frame << PAGE_SHIFT));
		}
	}
	else
	{
		*first2 = first1;
		*second2 = second1;
	}
	return 1;
}

// if virt_addr==0, finds a suitable address
void *ioremap_mm(u32 phys_addr, u32 size, u32 virt_addr, u32 flags, mm_t *mm)
{
	if (virt_addr & (PAGE_SIZE-1)) BUG();
	if (size == 0) return NULL;
	u32 frame = phys_addr >> PAGE_SHIFT;
	size += phys_addr & (PAGE_SIZE-1);
	size += PAGE_SIZE-1;
	u32 count = size >> PAGE_SHIFT;
	if (count > PAGE_SIZE/sizeof(pte_t)) return NULL;

	u8 page_flags = 0;
	if (flags & MAP_USERSPACE) page_flags |= PTE_USER;
	if (flags & MAP_READWRITE) page_flags |= PTE_WRITE;

	pte_t *dir = mm->page_dir;

	// should map count frames starting at frame
	pte_t *first = &dir[(virt_addr?virt_addr:KERNEL_VIRT_ADDR) / (4*1024*1024)];
	if (!(first->flags & PTE_PRESENT)) alloc_page_table_page(first);
	pte_t *second = (pte_t *)(PHYS_MEM + (first->frame<<PAGE_SHIFT)) + (virt_addr?(virt_addr>>PAGE_SHIFT)&0x03FF:0);
	while (first < dir + PAGE_SIZE/sizeof(pte_t))
	{
		if (!(second->flags & PTE_PRESENT))
		{
			u32 i;
			pte_t *old_first = first, *old_second = second, *new_first, *new_second;
			// start the loop from 1 since we know that second is not present
			for (i=1; i<count; i++)
			{
				if (!get_next_pte(old_first, old_second, &new_first, &new_second, mm)) return NULL;
				if (new_second == NULL)						// success
				{
					i=count;
					alloc_page_table_page(new_first);
					break;
				}
				if (new_second->flags & PTE_PRESENT) break;	// failure
				old_first = new_first;
				old_second = new_second;
			}
			if (i == count)									// found count free pages
			{
				// map them
				old_first = first;
				old_second = second;
				for (i=0; i<count; i++)
				{
					if (flags & MAP_USERSPACE) first->flags |= PTE_USER;
					second->flags = PTE_PRESENT | page_flags;
					second->frame = frame++;
					if (!get_next_pte(first, second, &new_first, &new_second, mm)) return NULL;
					// new_first was allocated if necessary, thus new_second!=NULL
					first = new_first;
					second = new_second;
				}
				u32 offset = phys_addr & (PAGE_SIZE-1);
				i = old_first - dir;
				u32 j = ((u32)old_second & (PAGE_SIZE-1)) / sizeof(pte_t);
				return (void *)((((i<<10) | j) << PAGE_SHIFT) | offset);
			}
			// if got here, then not found count free pages, and not allocated 2nd level page tables
		}
		pte_t *first2, *second2;
		if (!get_next_pte(first, second, &first2, &second2, mm)) return NULL;
		if (second2 == NULL)
		{
			alloc_page_table_page(first2);
			second2 = (pte_t *)(PHYS_MEM + (first2->frame << PAGE_SHIFT));
		}
		first = first2;
		second = second2;
	}
	return NULL;
}

// returns the physical address of the page pointed to by address
u32 __iounmap(void *address, u32 size)
{
	if (size == 0) return 0;
	u32 addr = (u32)address;
	u32 page = addr >> PAGE_SHIFT;
	size += addr & (PAGE_SIZE-1);
	size += PAGE_SIZE-1;
	u32 count = size >> PAGE_SHIFT;
	if (count > PAGE_SIZE/sizeof(pte_t)) BUG();
	int i = page>>10, j = page & 0x03FF;
	pte_t *first = &current->mm.page_dir[i];
	if (!(first->flags & PTE_PRESENT)) BUG();
	pte_t *second = (pte_t *)(PHYS_MEM + (first->frame<<PAGE_SHIFT)) + j;
	u32 ret = second->frame;
	while (count--)
	{
		if (!(second->flags & PTE_PRESENT)) BUG();
		second->flags &= ~PTE_PRESENT;
		pte_t *new_first, *new_second;
		if (!get_next_pte(first, second, &new_first, &new_second, &current->mm)) BUG();
		if (new_second == NULL) BUG();
		first = new_first;
		second = new_second;
	}
	return ret;
}

void *ioremap(u32 phys_addr, u32 size, u32 virt_addr, u32 flags)
{
	return ioremap_mm(phys_addr, size, virt_addr, flags, &current->mm);
}


void iounmap(void *address, u32 size)
{
	__iounmap(address, size);
}

u32 virt_to_phys_mm(void *address, mm_t *mm)
{
	u32 addr = (u32)address;
	u32 offset = addr & (PAGE_SIZE-1);
	addr >>= PAGE_SHIFT;
	u32 i = addr >> 10;
	u32 j = addr & 0x03FF;
	pte_t *first = &mm->page_dir[i];
	if (!(first->flags & PTE_PRESENT)) BUG();
	pte_t *second = (pte_t *)(PHYS_MEM + (first->frame<<PAGE_SHIFT)) + j;
	if (!(second->flags & PTE_PRESENT)) BUG();
	return (second->frame << PAGE_SHIFT) + offset;
}

u32 virt_to_phys(void *address)
{
	return virt_to_phys_mm(address, &current->mm);
}

/* allocated a page and adds the necessary mappings to the page tables, creating more if needed
   returns the virtual address of the newly allocated page */
void *alloc_page_mm(u32 flags, mm_t *mm)
{
#if 0
	u32 phys = alloc_phys_page();
	if (phys == 0) return NULL;
	return ioremap_mm(phys << PAGE_SHIFT, PAGE_SIZE, 0, flags, mm);
#else
	return 0;
#endif
}

void *alloc_page(u32 flags)
{
	return alloc_page_mm(flags, &current->mm);
}

// allocated count consecutive pages and maps them to the vma
void *alloc_pages(u32 count, u32 flags)
{
	if (count == 0) return NULL;
	if (count == 1) return alloc_page(flags);
	u32 phys = alloc_phys_pages(count);
	if (phys == 0) return NULL;
	return ioremap(phys << PAGE_SHIFT, count << PAGE_SHIFT, 0, flags);
}

void free_page_mm(void *address, mm_t *mm)
{
#if 0
	u32 addr = (u32)address;
	int i, j;
	pte_t *first, *second;
	if (addr & (PAGE_SIZE-1)) BUG();
	addr >>= PAGE_SHIFT;
	i = addr >> 10;
	j = addr & ((1<<10) - 1);
	first = &mm->page_dir[i];
	if (!(first->flags & PTE_PRESENT)) BUG();
	second = (pte_t *)(PHYS_MEM + (first->frame*PAGE_SIZE)) + j;
	if (!(second->flags & PTE_PRESENT)) BUG();
	second->flags &= ~PTE_PRESENT;
	free_phys_page(second->frame);
#endif
}

void free_page(void *address)
{
	free_page_mm(address, &current->mm);
}

void free_pages(void *address, u32 count)
{
	u32 __iounmap(void *address, u32 size);

	if (count == 0) return;
	if (count == 1) free_page(address);
	if ((u32)address & (PAGE_SIZE-1)) BUG();
	u32 frame = __iounmap(address, count << PAGE_SHIFT);
	if (frame == 0) BUG();
	free_phys_pages(frame, count);
}

int clone_mm(mm_t *dest_mm)
{
	pte_t *old_pg = current->mm.page_dir, *new_pg = alloc_page(MAP_READWRITE);
	if (!new_pg) return 0;
	memcpy(new_pg, old_pg, PAGE_SIZE);
	dest_mm->page_dir = new_pg;
	int i, j;
	for (i=0; i<KERNEL_VIRT_ADDR/(4*1024*1024); i++)
	{
		if (!(old_pg[i].flags & PTE_PRESENT)) continue;
		pte_t *old_second = (pte_t *)(PHYS_MEM + (old_pg[i].frame<<PAGE_SHIFT));
		new_pg[i].flags = 0;
		if (alloc_page_table_page(&new_pg[i]) == 0) return 0;
		new_pg[i].flags |= PTE_USER;
		pte_t *new_second = (pte_t *)(PHYS_MEM + (new_pg[i].frame<<PAGE_SHIFT));
		memcpy(new_second, old_second, PAGE_SIZE);
		for (j=0; j<PAGE_SIZE/sizeof(pte_t); j++)
		{
			if (!(new_second[j].flags & PTE_PRESENT)) continue;
			void *old_page = (void *)(PHYS_MEM + (old_second[j].frame<<PAGE_SHIFT));
			void *new_page = alloc_page_mm(MAP_USERSPACE | (old_second[j].flags&PTE_WRITE ? MAP_READWRITE : 0), dest_mm);
			if (!new_page) return 0;
			new_second->frame = virt_to_phys_mm(new_page, dest_mm);
			memcpy(new_page, old_page, PAGE_SIZE);
		}
	}
	return 1;
}

void free_mm(mm_t *mm)
{
	int i, j;
	pte_t *dir = mm->page_dir;
	for (i=0; i<KERNEL_VIRT_ADDR/(4*1024*1024); i++)
	{
		if (!(dir[i].flags & PTE_PRESENT)) continue;
		pte_t *second = (pte_t *)(PHYS_MEM + (dir[i].frame<<PAGE_SHIFT));
		for (j=0; j<PAGE_SIZE/sizeof(pte_t); j++)
		{
			if (!(second[j].flags & PTE_PRESENT)) continue;
//			free_phys_page(second[j].frame);
		}
//		free_phys_page(dir[i].frame);
	}
//	free_phys_page(virt_to_phys(page_dir));
}
#endif

#if 0
void init_task_mm(mm_t *mm)
{
	mm->page_dir = page_dir;
}
#endif

page_t *pages;

void init_mm(u32 size)
{
	int i;
	size >>= PAGE_SHIFT;
	pages = kmalloc(size * sizeof(page_t));
	for (i = 0; i < size; i++)
		pages[i].count = -1;
}

int clone_mm(task_t *p)
{
	mm_t *mm = &p->mm;
	
	/* Copy page tables */
	pte_t *old_pg = current->mm.page_dir, *new_pg = alloc_page(MAP_READWRITE);
	if (!new_pg)
		goto out;
	memcpy(new_pg, old_pg, PAGE_SIZE);
	mm->page_dir = new_pg;
	int i, j;
	for (i=0; i<KERNEL_VIRT_ADDR/(4*1024*1024); i++)
	{
		if (!(old_pg[i].flags & PTE_PRESENT))
			continue;
		pte_t *old_second = (pte_t *)P2V(old_pg[i].frame<<PAGE_SHIFT);
		pte_t *new_second = alloc_page(MAP_READWRITE);
		BUG_ON(!new_second);
		new_pg[i].frame = V2P((u32)new_second) >> PAGE_SHIFT;
		memcpy(new_second, old_second, PAGE_SIZE);
		for (j=0; j < PAGE_SIZE/sizeof(pte_t); j++)
		{
			if (!(new_second[j].flags & PTE_PRESENT))
				continue;
			page_t *page = &pages[new_second[j].frame];
			BUG_ON(page->count <= 0);
			page->count++;
		}
	}

	/* Copy vm_areas */
	list_t *vm_area_iter;
	list_init(&mm->vm_areas);
	list_for_each(&current->mm.vm_areas, vm_area_iter) {
		vm_area_t *vm_area = list_get(vm_area_iter, vm_area_t, list);
		vm_area_t *new_area = kzalloc(sizeof(vm_area_t));
		BUG_ON(!new_area);
		memcpy(new_area, vm_area, sizeof(vm_area_t));
		list_add_tail(&mm->vm_areas, &new_area->list);
	}
	return 0;
out:
	return -ENOMEM;
}

int mm_add_area(u32 start, u32 size, u32 flags)
{
	list_t *vm_area_iter;
	list_for_each(&current->mm.vm_areas, vm_area_iter) {
		vm_area_t *vm_area = list_get(vm_area_iter, vm_area_t, list);
		if (start < vm_area->start)
			break;
	}
	vm_area_t *new_area = kzalloc(sizeof(vm_area_t));
	if (!new_area)
		return -ENOMEM;
	new_area->start = start;
	new_area->end = start + size;
	new_area->flags = flags;
	list_add_before(vm_area_iter, &new_area->list);
	return 0;
}
