#include <mm.h>
#include <string.h>
#include <kernel.h>

static u64 *bitmap;
static u64 first_frame;
static u64 bitmap_size;			/* in bytes */
static u64 memsize, memused;		/* in bytes */

void init_pmm(u64 physbase, u64 size)
{
	int i;

	/* base and size are in bytes */
	BUG_ON((physbase & ~PAGE_MASK) != 0);
	BUG_ON((size & ~PAGE_MASK) != 0);

	bitmap = (u64 *)P2V(physbase);
	first_frame = physbase >> PAGE_SHIFT;

	memsize = size;
	bitmap_size = (memsize >> PAGE_SHIFT) / 8;

	memset(bitmap, 0, bitmap_size);
	memused = 0;

	/* Mark in bitmap that the bitmap itself is used */
	for (i = 0; i < ROUND_PAGE_UP(bitmap_size) >> PAGE_SHIFT; i++) {
		u32 frame = alloc_phys_page();
		BUG_ON(frame != first_frame + i);
	}
	printk("bitmap=%p\nbitmap_size=%ld\nfirst_frame=%ld\nmemsize=%ld\nmemused=%ld\n", bitmap, bitmap_size, first_frame, memsize, memused);
}

/* returns frame number of allocated frame */
u64 alloc_phys_page(void)
{
	u64 i;
	u64 bit = 1, bit_offs = 0;
	for (i = 0; i < bitmap_size/8; i++) {
		if (bitmap[i] != -1UL) break;
	}
	if (i == bitmap_size/8) return 0;	/* no free memory */
	/* not an endless loop, since free space exists */
	while (bitmap[i] & bit)	{
		bit <<= 1;
		bit_offs++;
	}
	bitmap[i] |= bit;			/* mark it as used */
	memused += PAGE_SIZE;
	u64 frame = first_frame + i*64 + bit_offs;	/* 64 == bits in long */
	if (pages) {
		page_t *page = &pages[frame];
		BUG_ON(page->count > 0);
		page->count = 1;
	}
	return frame;
}

/* frees frame by number */
void free_phys_page(u64 frame)
{
	u64 i, bit;
	u64 orig_frame = frame;
	frame -= first_frame;
	BUG_ON(frame/8 >= bitmap_size);
	i = frame / 64;				/* 64 == bits in long */
	bit = 1UL << (frame % 64);
	BUG_ON((bitmap[i] & bit) == 0);		/* page is free */
	bitmap[i] &= ~bit;
	memused -= PAGE_SIZE;
	page_t *page = &pages[orig_frame];
	BUG_ON(page->count != 1);
	page->count = 0;
}

u64 get_mem_size(void)
{
	return memsize;
}

u64 get_used_mem(void)
{
	return memused;
}

u64 get_free_mem(void)
{
	return memsize - memused;
}
