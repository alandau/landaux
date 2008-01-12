#ifndef MEM_H
#define MEM_H

#define PAGE_SHIFT		12
#define PAGE_SIZE		4096
#define PAGE_SIZE_K		(PAGE_SIZE/1024)
#define KERNEL_VIRT_ADDR	0xC0000000
#define KERNEL_PHYS_ADDR	0x100000

/* Don't parenthesize addr, since the macros are used in assembly code */
#define P2V(addr)		(addr + KERNEL_VIRT_ADDR)
#define V2P(addr)		(addr - KERNEL_VIRT_ADDR)

#define PTE_PRESENT		0x01
#define PTE_WRITE		0x02
#define PTE_USER		0x04
#define PTE_ACCESSED		0x20
#define PTE_DIRTY		0x40

#endif
