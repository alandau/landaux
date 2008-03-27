#include <mm.h>
#include <string.h>
#include <kernel.h>

static u32 *bitmap;
static u32 bitmap_size;	/* in bytes */
static u32 first_frame;
static u32 memsize;		/* in pages */
static u32 memused;		/* in bytes */

void init_pmm(u32 base, u32 size)
{
	/* base and size are in bytes */
/* This is only legal since the arguments are below 4MB */
#define P2V(addr)		((addr) + KERNEL_VIRT_ADDR)
#define V2P(addr)		((addr) - KERNEL_VIRT_ADDR)
	bitmap = (u32 *)P2V(base);
	bitmap_size = size / PAGE_SIZE / 8;
	first_frame = ROUND_PAGE_UP(base + bitmap_size) >> PAGE_SHIFT;
	memsize = (size - ROUND_PAGE_UP(bitmap_size)) / PAGE_SIZE;
	BUG_ON(first_frame*PAGE_SIZE >= 4*1024*1024);	/* bitmap should be mapped by first 4MB */
	memset(bitmap, 0, bitmap_size);
	memused = first_frame << PAGE_SHIFT;
	printk("bitmap=%x\nbitmap_size=%d\nfirst_frame=%d\nmemsize=%d\nmemused=%d\n", bitmap, bitmap_size, first_frame, memsize, memused);
#undef P2V
#undef V2P
}

/* returns frame number of allocated frame */
u32 alloc_phys_page(void)
{
	int i, bit = 1, bit_offs = 0;
	for (i = 0; i < bitmap_size/4; i++) {
		if (bitmap[i] != 0xFFFFFFFF) break;
	}
	if (i == bitmap_size/4) return 0;	/* no free memory */
	/* not an endless loop, since free space exists */
	while (bitmap[i] & bit)	{
		bit <<= 1;
		bit_offs++;
	}
	bitmap[i] |= bit;			/* mark it as used */
	memused += PAGE_SIZE;
	return first_frame + i*32 + bit_offs;	/* 32 == bits in long */
}

/* frees frame by number */
void free_phys_page(u32 frame)
{
	int bit_offs, bit, i;
	frame -= first_frame;
	i = frame / 32;			/* 32 == bits in long */
	BUG_ON(i >= bitmap_size);
	bit_offs = frame % 32;
	bit = 1 << bit_offs;
	BUG_ON((bitmap[i] & bit) == 0);		/* page is free */
	bitmap[i] &= ~bit;
	memused -= PAGE_SIZE;
}
