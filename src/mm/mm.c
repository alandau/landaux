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
unsigned long alloc_phys_page(void)
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

/* frees frame by number */
void free_phys_page(unsigned long frame)
{
	int bit_offs, bit, i;
	i = frame / 32;			/* 4 == sizeof(unsigned long) */
	bit_offs = frame % 32;
	bit = 1 << bit_offs;
	if ((bitmap[i] & bit) == 0)		/* page is free */
		BUG();
	bitmap[i] &= ~bit;
	memused -= PAGE_SIZE;
}

void init_mm(void)
{
	unsigned long i, kernel_pages;
	memsize = __get_memsize();
	memused = 0;
	extern unsigned char code[], end[];
	printk("Found %u MB of memory.\n", memsize/1024/1024);
	kernel_size = end-code;
	printk("Kernel takes up %u bytes.\n", kernel_size);
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
			unsigned long table = alloc_phys_page();
			if (!table) BUG();
			if (table >= ONE_MB) BUG();
			first->flags = PTE_PRESENT | PTE_WRITE;
			first->frame = table;
		}
		// first->frame < 1MB, thus phys_addr=virt_addr
		second = (pte_t *)(first->frame*PAGE_SIZE) + (i%1024);
		second->flags = PTE_PRESENT | PTE_WRITE;
		second->frame = i;
	}
}

/* allocated a page and adds the necessary mappings to the page tables, creating more if needed
   returns the virtual address of the newly allocated page */
void *alloc_page(void)
{
	int i, j;
	unsigned long phys = alloc_phys_page();
	if (phys == 0) return NULL;
	for (i=KERNEL_VIRT_ADDRESS/(4*1024*1024); i<KERNEL_VIRT_ADDRESS/(4*1024*1024) + PAGE_SIZE/sizeof(pte_t); i++)
	{
		pte_t *first = &page_dir[i];
		if ((!first->flags & PTE_PRESENT))
		{
			// allocate second-level page table
			unsigned long table = alloc_phys_page();
			if (!table) return NULL;
			first->flags = PTE_PRESENT | PTE_WRITE;
			first->frame = table;

		}
		for (j=0; j<PAGE_SIZE/sizeof(pte_t); j++)
		{
			pte_t *second = (pte_t *)(PHYS_MEM + (first->frame*PAGE_SIZE)) + j;
			if (!(second->flags & PTE_PRESENT))
			{
				second->flags = PTE_PRESENT | PTE_WRITE;
				second->frame = phys;
				return (void *)(((i<<10)|j) << PAGE_SHIFT);
			}
		}
	}
	return NULL;
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
