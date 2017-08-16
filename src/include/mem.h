#ifndef MEM_H
#define MEM_H

#define PAGE_SHIFT		12
#define PAGE_SIZE		4096
#define PAGE_SIZE_K		(PAGE_SIZE/1024)
#define PAGE_MASK		(~(PAGE_SIZE-1))

#define MEM_TOP_HALF		0xffff800000000000
#define MEM_BOTTOM_HALF		0x0000800000000000
#define KERNEL_VIRT_ADDR	0xffffffff80000000
#define KERNEL_PHYS_ADDR	0x100000
#define MEMORY_BASE		0xffffff0000000000
#define HEAP_START		0xfffffe0000000000

#define ROUND_PAGE_DOWN(x)	((x) & PAGE_MASK)
#define ROUND_PAGE_UP(x)	(((x) + PAGE_SIZE-1) & PAGE_MASK)

#define P2V_IMAGE(addr)		((addr) + KERNEL_VIRT_ADDR)
#define V2P_IMAGE(addr)		((addr) - KERNEL_VIRT_ADDR)
#define P2V(addr)		((void*)((u64)(addr) + MEMORY_BASE))
#define V2P(addr)		((u64)(addr) - MEMORY_BASE)

#define PTE_PRESENT		0x01
#define PTE_WRITE		0x02
#define PTE_USER		0x04
#define PTE_ACCESSED		0x20
#define PTE_DIRTY		0x40
#define PTE_SIZE		0x80

#endif
