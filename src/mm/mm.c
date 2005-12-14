#include <mm.h>
#include <arch.h>
#include <video.h>
#include <string.h>
#include <kernel.h>

#define CMOS_REG		0x70
#define CMOS_DATA		0x71

#define ONE_MB			0x100000

#define PTE_PRESENT		0x01
#define PTE_WRITE		0x02
#define PTE_USER		0x04
#define PTE_ACCESSED	0x20
#define PTE_DIRTY		0x40

#define PHYS_MEM				0xF0000000
#define KERNEL_VIRT_ADDRESS		0xC0000000
#define MAX_MEM_SIZE			(256*1024*1024)	/* used to calculate bitmap size */

typedef struct pte_struct		/* page table entry */
{
	unsigned char flags;
	unsigned reserved : 4;		/* bits 8-12, includes Intel-Reserved and Avail */
	unsigned frame : 20;
} pte_t;

static unsigned long memsize;		/* in bytes */
static unsigned long memused;		/* in bytes */
static unsigned long bitmap[MAX_MEM_SIZE / PAGE_SIZE / 32];
static int bitmap_size;				/* number of unsigned long's in mem_bitmap */
static unsigned long kernel_size;	/* in bytes */

pte_t page_dir[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t kernel_page_table[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
pte_t first_mb_table[PAGE_SIZE/sizeof(pte_t)] __attribute__ ((aligned (PAGE_SIZE)));
unsigned long stack[1024] __attribute__ ((aligned (PAGE_SIZE)));

/* in bytes */
static unsigned long __get_memsize(void)
{
	unsigned short total;
	unsigned char tmp;
	outb(CMOS_REG, 0x18);
	tmp = inb(CMOS_DATA);
	total = tmp << 8;
	outb(CMOS_REG, 0x17);
	tmp = inb(CMOS_DATA);
	total += tmp;
	return (total + 1024) * 1024;
}

/* returns frame number of allocated frame */
static unsigned long alloc_phys_page(void)
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

static int have_enough_zeros(unsigned long *p, unsigned long start_bit, unsigned long max_bits, int zeros)
{
	for (; zeros && max_bits; max_bits--, zeros--)
	{
		if (p[start_bit/32] & (1<<(start_bit%32))) return 0;
		start_bit++;
	}
	return (zeros == 0);
}

// allocates count consecutive frames
static unsigned long alloc_phys_pages(unsigned long count)
{
	int i, bit;
	if (count == 0) return 0;
	for (i=0; i < bitmap_size; i++)
	{
		if (bitmap[i] == 0xFFFFFFFF) continue;
		for (bit = 0; bit < 32; bit++)
		{
			if (!have_enough_zeros(bitmap, i*32 + bit, bitmap_size*32, count)) continue;
			unsigned long pos = i*32;
			unsigned long ret = pos + bit;
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

static void free_phys_pages(unsigned long frame, unsigned long count)
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
static unsigned long alloc_zeroed_phys_page(void)
{
	unsigned long frame = alloc_phys_page();
	if (!frame) return 0;
	memset((void *)(PHYS_MEM + frame*PAGE_SIZE), 0, PAGE_SIZE);
	return frame;
}

/* frees frame by number */
static void free_phys_page(unsigned long frame)
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

/* allocates a pagetable page to be pointed by first
   returns the allocated frame number */
static unsigned long alloc_page_table_page(pte_t *first)
{
	if (first->flags & PTE_PRESENT) BUG();
	unsigned long table = alloc_zeroed_phys_page();
	if (!table) BUG();
	first->flags = PTE_PRESENT | PTE_WRITE;
	first->frame = table;
	return table;
}

void init_mm(void)
{
	unsigned long i, kernel_pages;
	memsize = __get_memsize();
	memused = 0;
	extern unsigned char code[], end[];
	kernel_size = end-code;
	/* Assume memory size is a multiple of 128K */
	bitmap_size = (memsize / PAGE_SIZE) / 32;
	/* bitmap is in BSS and thus already zeroed */

	/* mark that the memory used by kernel: start at 0x1000=4KB, end at 'end'
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
			unsigned long table = alloc_phys_page();
			if (!table || (table<<PAGE_SHIFT) >= ONE_MB) BUG();
			first->flags = PTE_PRESENT | PTE_WRITE;
			first->frame = table;
			// zero it
			memset((void *)(table<<PAGE_SHIFT), 0, PAGE_SIZE);
		}
		// first->frame < 1MB, thus phys_addr=virt_addr
		second = (pte_t *)(first->frame*PAGE_SIZE) + (i%1024);
		second->flags = PTE_PRESENT | PTE_WRITE;
		second->frame = i;
	}

	// unmap the first mb
//	page_dir[0].flags &= ~PTE_PRESENT;
//	int i = ((unsigned long)first_mb_table - KERNEL_VIRT_ADDRESS) >> PAGE_SHIFT;
//	bitmap[i/32] &= ~(1 << (i%32));
	// for now, unmap first page, to catch NULL dereferences
	pte_t *second = (pte_t *)(PHYS_MEM+page_dir[0].frame*PAGE_SIZE);
	second->flags &= ~PTE_PRESENT;
}

/* allocated a page and adds the necessary mappings to the page tables, creating more if needed
   returns the virtual address of the newly allocated page */
void *alloc_page(void)
{
	unsigned long phys = alloc_phys_page();
	if (phys == 0) return NULL;
	return ioremap(phys << PAGE_SHIFT, PAGE_SIZE);
}

// allocated count consecutive pages and maps them to the vma
void *alloc_pages(unsigned long count)
{
	if (count == 0) return NULL;
	if (count == 1) return alloc_page();
	unsigned long phys = alloc_phys_pages(count);
	if (phys == 0) return NULL;
	return ioremap(phys << PAGE_SHIFT, count << PAGE_SHIFT);
}

void free_page(void *address)
{
	unsigned long addr = (unsigned long)address;
	int i, j;
	pte_t *first, *second;
	if (addr & (PAGE_SIZE-1)) BUG();
	addr >>= PAGE_SHIFT;
	i = addr >> 10;
	j = addr & ((1<<10) - 1);
	first = &page_dir[i];
	if (!(first->flags & PTE_PRESENT)) BUG();
	second = (pte_t *)(PHYS_MEM + (first->frame*PAGE_SIZE)) + j;
	if (!(second->flags & PTE_PRESENT)) BUG();
	second->flags &= ~PTE_PRESENT;
	free_phys_page(second->frame);
}

void free_pages(void *address, unsigned long count)
{
	unsigned long __iounmap(void *address, unsigned long size);

	if (count == 0) return;
	if (count == 1) free_page(address);
	if ((unsigned long)address & (PAGE_SIZE-1)) BUG();
	unsigned long frame = __iounmap(address, count << PAGE_SHIFT);
	if (frame == 0) BUG();
	free_phys_pages(frame, count);
}




unsigned long get_mem_size()
{
	return memsize;
}

unsigned long get_used_mem()
{
	return memused;
}

unsigned long get_free_mem()
{
	return memsize - memused;
}


// given the pte at (first1, second1), find the next pte and put it's address at (*first2, *second2)
// *second2 will be NULL if *first2 points to a not-yet-allocated page
// returns 0 if *first2 went out of page_dir, 1 otherwise
int get_next_pte(pte_t *first1, pte_t *second1, pte_t **first2, pte_t **second2)
{
	second1++;
	if (!((unsigned long)second1 & (PAGE_SIZE-1)))		// increment first
	{
		*first2 = first1+1;
		*second2 = NULL;
		if (*first2 - page_dir >= PAGE_SIZE/sizeof(pte_t)) return 0;
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


void *ioremap(unsigned long phys_addr, unsigned long size)
{
	if (size == 0) return NULL;
	unsigned long frame = phys_addr >> PAGE_SHIFT;
	size += phys_addr & (PAGE_SIZE-1);
	size += PAGE_SIZE-1;
	unsigned long count = size >> PAGE_SHIFT;
	if (count > PAGE_SIZE/sizeof(pte_t)) return NULL;

	// should map count frames starting at frame
	pte_t *first = &page_dir[KERNEL_VIRT_ADDRESS/(4*1024*1024)];
	if (!(first->flags & PTE_PRESENT)) alloc_page_table_page(first);
	pte_t *second = (pte_t *)(PHYS_MEM + (first->frame<<PAGE_SHIFT));
	while (first < page_dir + PAGE_SIZE/sizeof(pte_t))
	{
		if (!(second->flags & PTE_PRESENT))
		{
			unsigned long i;
			pte_t *old_first = first, *old_second = second, *new_first, *new_second;
			// start the loop from 1 since we know that second is not present
			for (i=1; i<count; i++)
			{
				if (!get_next_pte(old_first, old_second, &new_first, &new_second)) return NULL;
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
					second->flags = PTE_PRESENT | PTE_WRITE;
					second->frame = frame++;
					if (!get_next_pte(first, second, &new_first, &new_second)) return NULL;
					// new_first was allocated if necessary, thus new_second!=NULL
					first = new_first;
					second = new_second;
				}
				unsigned long offset = phys_addr & (PAGE_SIZE-1);
				i = old_first - page_dir;
				unsigned long j = ((unsigned long)old_second & (PAGE_SIZE-1)) / sizeof(pte_t);
				return (void *)((((i<<10) | j) << PAGE_SHIFT) | offset);
			}
			// if got here, then not found count free pages, and not allocated 2nd level page tables
		}
		pte_t *first2, *second2;
		if (!get_next_pte(first, second, &first2, &second2)) return NULL;
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
unsigned long __iounmap(void *address, unsigned long size)
{
	if (size == 0) return 0;
	unsigned long addr = (unsigned long)address;
	unsigned long page = addr >> PAGE_SHIFT;
	size += addr & (PAGE_SIZE-1);
	size += PAGE_SIZE-1;
	unsigned long count = size >> PAGE_SHIFT;
	if (count > PAGE_SIZE/sizeof(pte_t)) BUG();
	int i = page>>10, j = page & 0x03FF;
	pte_t *first = &page_dir[i];
	if (!(first->flags & PTE_PRESENT)) BUG();
	pte_t *second = (pte_t *)(PHYS_MEM + (first->frame<<PAGE_SHIFT)) + j;
	unsigned long ret = second->frame;
	while (count--)
	{
		if (!(second->flags & PTE_PRESENT)) BUG();
		second->flags &= ~PTE_PRESENT;
		pte_t *new_first, *new_second;
		if (!get_next_pte(first, second, &new_first, &new_second)) BUG();
		if (new_second == NULL) BUG();
		first = new_first;
		second = new_second;
	}
	return ret;
}

void iounmap(void *address, unsigned long size)
{
	__iounmap(address, size);
}
