#ifndef MEM_H
#define MEM_H

#define PAGE_SHIFT		12
#define PAGE_SIZE		4096
#define PAGE_SIZE_K		(PAGE_SIZE/1024)
#define PAGE_MASK		(~(PAGE_SIZE-1))
#define KERNEL_VIRT_ADDR	0xC0000000
#define KERNEL_PHYS_ADDR	0x100000

#define ROUND_PAGE_DOWN(x)	((x) & PAGE_MASK)
#define ROUND_PAGE_UP(x)	(((x) + PAGE_SIZE-1) & PAGE_MASK)

#define P2V(addr)		((addr) + KERNEL_VIRT_ADDR)
#define V2P(addr)		((addr) - KERNEL_VIRT_ADDR)

#define PTE_PRESENT		0x01
#define PTE_WRITE		0x02
#define PTE_USER		0x04
#define PTE_ACCESSED		0x20
#define PTE_DIRTY		0x40

#endif
