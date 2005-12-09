#include <mm.h>
#include <arch.h>
#include <video.h>
#include <string.h>
#include <kernel.h>

#define CMOS_REG	0x70
#define CMOS_DATA	0x71

#define CR0_PG_BIT	0x80000000

#define MAX_EXT_MEM_SIZE	(31*1024*1024)		/* 31MB */
#define FIRST_PAGE	0x100000

#define PTE_PRESENT	0x01
#define PTE_WRITE	0x02
#define PTE_USER	0x04
#define PTE_ACCESSED	0x20
#define PTE_DIRTY	0x40

#define NUM_PTE		(PAGE_SIZE / sizeof(pte_t))

#define IDENTITY_MAP	(1024*1024)	/* 1MB */

#define PHYS_MEM	0xF0000000
#define MEM_SIZE	(32*1024*1024)

typedef struct pte_struct		/* page table entry */
{
	unsigned char flags;
	unsigned reserved : 4;		/* bits 8-12, includes Intel-Reserved and Avail */
	unsigned frame : 20;
} pte_t;

static unsigned long memsize;
static unsigned long bitmap[MAX_EXT_MEM_SIZE / PAGE_SIZE / sizeof(long) / 8];
static int bitmap_size;				/* number of unsigned long's in mem_bitmap*/

static pte_t *page_dir = (pte_t *)(PHYS_MEM + FIRST_PAGE);

/* in Kbytes */
static unsigned long get_memsize(void)
{
	unsigned short total;
	unsigned char tmp;
	outb(CMOS_REG, 0x18);
	tmp = inb(CMOS_DATA);
	total = tmp << 8;
	outb(CMOS_REG, 0x17);
	tmp = inb(CMOS_DATA);
	total += tmp;
//	if (total > MAX_EXT_MEM_SIZE / 1024) total = MAX_EXT_MEM_SIZE / 1024;
	return total + 1024;
}

void *alloc_phys_page(void)
{
	int i, bit = 1, bit_offs = 0;
	for (i = 0; i < bitmap_size; i++)
	{
		if (bitmap[i] != 0xFFFFFFFF) break;
	}
	if (i == bitmap_size) return NULL;	/* no free memory */
	/* not an endless loop, since free space exists */
	while (bitmap[i] & bit)
	{
		bit <<= 1;
		bit_offs++;
	}
	bitmap[i] |= bit;			/* mark it as used */
	return (void *)(FIRST_PAGE + PAGE_SIZE * (i*32 + bit_offs));	/* 32 == bits in long */
}

void free_phys_page(void *address)
{
	int bit_offs, bit = 1, i;
	if ((unsigned long)address < FIRST_PAGE) BUG();
	bit_offs = (unsigned long)((char *)address - FIRST_PAGE) / PAGE_SIZE;
	i = bit_offs / 32;			/* 4 == sizeof(unsigned long) */
	bit_offs %= 32;
	while (bit_offs--) bit <<= 1;
	if ((bitmap[i] & bit) == 0)		/* page is free */
		BUG();
	bitmap[i] &= ~bit;
}

void init_mm(void)
{
	int i;
	memsize = MEM_SIZE; /*get_memsize();*/
	printk("memsize=%u\n", get_memsize());
	bitmap_size = ((memsize - 1024) / PAGE_SIZE_K) / 4;
	/* bitmap is in BSS and thus already zeroed */
	/* mark that the first memsize/PAGE_SIZE + 1 pages as used:
	   1 for page dir, other are page tables */
	for (i=0; i<memsize/PAGE_SIZE + 1; i++)
		bitmap[i/32] |= 1 << (i%32);
}

void *alloc_page(void)
{
	void *phys = alloc_phys_page();
	if (phys == NULL) return NULL;
}

void free_page(void *address)
{
}
