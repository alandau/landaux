#include <mm.h>
#include <kernel.h>

#define HEAP_START 0xE0000000

static u8 *heap_top;

/* bit 0 of size: 1=used, 0=free*/
typedef struct node {
	u32 size;
} node_t;

void map_page(u32 frame, u32 addr);

void init_heap(void)
{
	u32 frame;
	node_t *n;
	heap_top = (void *)HEAP_START;
	frame = alloc_phys_page();
	BUG_ON(frame == 0);
	map_page(frame, (u32)heap_top);
	heap_top += PAGE_SIZE;
	n = (node_t *)HEAP_START;
	n->size = PAGE_SIZE - sizeof(node_t);
}

void *kmalloc(u32 size)
{
	node_t *n = (node_t *)HEAP_START, *last_free_node = NULL;
	size = (size + 3) / 4 * 4;	/* align to 4 bytes */
	while (1) {
		if ((u8 *)n >= heap_top) {
			u32 frame;
			printk("n=%x, heap_top=%x\n", n, heap_top);
			BUG_ON((u8 *)n > heap_top);
			frame = alloc_phys_page();
			if (frame == 0) {
				printk("Allocation of %d bytes failed!\n", size);
				return NULL;
			}
			map_page(frame, (u32)heap_top);
			heap_top += PAGE_SIZE;
			if (last_free_node) {
				last_free_node->size += PAGE_SIZE;
				n = last_free_node;
			} else {
				n->size = PAGE_SIZE;
			}
			continue;
		}
		if (n->size & 1) {	/* used */
			n = (node_t *)((u32)n + sizeof(node_t) + (n->size & ~1));
			last_free_node = NULL;
			continue;
		}
		if (n->size >= size + sizeof(node_t)) {		/* enough free space for size and a new node */
			u8 *p = (u8 *)n + sizeof(node_t);
			node_t *new_node = (node_t *)(p + size);
			new_node->size = n->size - size - sizeof(node_t);
			n->size = size | 1;
			return p;
		} else {					/* free block smaller than required */
			last_free_node = n;
			n = (node_t *)((u32)n + sizeof(node_t) + n->size);
			continue;
		}
	}
}
