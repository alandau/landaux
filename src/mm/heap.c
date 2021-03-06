#include <mm.h>
#include <kernel.h>
#include <string.h>

static u8 *heap_top;

/* bit 0 of size: 1=used, 0=free*/
typedef struct node {
	u32 size;
} node_t;

void init_heap(void)
{
	u64 frame = alloc_phys_page();
	BUG_ON(frame == 0);
	heap_top = (void *)HEAP_START;
	map_kernel_page(frame, (u64)heap_top);
	heap_top += PAGE_SIZE;
	node_t *n = (node_t *)HEAP_START;
	n->size = PAGE_SIZE - sizeof(node_t);
}

void *kmalloc(u32 size)
{
	node_t *n = (node_t *)HEAP_START, *last_free_node = NULL;
	size = (size + 7) / 8 * 8;	/* align to 8 bytes */
	while (1) {
		if ((u8 *)n >= heap_top) {
			u32 frame;
			BUG_ON((u8 *)n > heap_top);
			frame = alloc_phys_page();
			if (frame == 0) {
				printk("Allocation of %d bytes failed!\n", size);
				print_stack(NULL);
				return NULL;
			}
			map_kernel_page(frame, (u64)heap_top);
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
			n = (node_t *)((u64)n + sizeof(node_t) + (n->size & ~1));
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
			/* try to merge following free blocks */
			u32 increase = 0;
			node_t *next_node = n; 
			while (1) {
				next_node = (node_t *)((u64)next_node + sizeof(node_t) + next_node->size);
				if ((u64)next_node < (u64)heap_top && (next_node->size & 1) == 0)
					increase += sizeof(node_t) + next_node->size;
				else
					break;
			}
			if (increase) {
				n->size += increase;
				continue;
			}
			last_free_node = n;
			n = (node_t *)((u64)n + sizeof(node_t) + n->size);
			continue;
		}
	}
}

void kfree(void *p)
{
	node_t *next_node;
	if (p == NULL)
		return;
	node_t *node = (node_t *)((u64)p - sizeof(node_t));
	BUG_ON((node->size & 1) == 0);
	node->size &= ~1;	/* mark as free */
	while (1) {
		next_node = (node_t *)((u64)node + sizeof(node_t) + node->size);
		/* if next node exists and is free, merge them */
		if ((u64)next_node < (u64)heap_top && (next_node->size & 1) == 0)
			node->size += sizeof(node_t) + next_node->size;
		else
			break;
	}

}

void *kzalloc(u32 size)
{
	void *p = kmalloc(size);
	if (p)
		memset(p, 0, size);
	return p;
}
