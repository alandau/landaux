#include <multiboot.h>
#include <mem.h>

.code32

.text

.align 4
/* Multiboot header */
mb_header:
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
.long V2P_IMAGE(mb_header)	/* header_addr */
.long V2P_IMAGE(_code)		/* load_addr */
.long V2P_IMAGE(_data_end)	/* load_end_addr */
.long V2P_IMAGE(_bss_end)	/* bss_end_addr */
.long V2P_IMAGE(_start)		/* entry_addr */

.global _start
_start:
	/* %eax holds MULTIBOOT_BOOTLOADER_MAGIC, %ebx points to multiboot_info_t */

	/* Initialize stack to be able to initialize %eflags */
	movl $(V2P_IMAGE(idle_task_stack) + PAGE_SIZE), %esp

	/* Initialize %eflags */
	pushl $0
	popfl

	/* Put multiboot magic aside */
	movl %eax, %esi

	/* Identity map first 4MB (also map them to KERNEL_VIRT_ADDR) */
	/* This should cover the whole kernel binary (code+data+bss) */
	movl $(0 + PTE_PRESENT + PTE_WRITE + PTE_SIZE), V2P_IMAGE(page_table_l2)
	movl $(0x200000 + PTE_PRESENT + PTE_WRITE + PTE_SIZE), V2P_IMAGE(page_table_l2 + 8)

	movl $(V2P_IMAGE(page_table_l2) + PTE_PRESENT + PTE_WRITE), V2P_IMAGE(page_table_l3)
	movl $(V2P_IMAGE(page_table_l2) + PTE_PRESENT + PTE_WRITE), V2P_IMAGE(page_table_l3 + ((KERNEL_VIRT_ADDR >> 30) & 0x1ff)*8)
	movl $(V2P_IMAGE(page_table_l3) + PTE_PRESENT + PTE_WRITE), V2P_IMAGE(page_table_l4)
	movl $(V2P_IMAGE(page_table_l3) + PTE_PRESENT + PTE_WRITE), V2P_IMAGE(page_table_l4 + ((KERNEL_VIRT_ADDR >> 39) & 0x1ff)*8)
	
	movl $V2P_IMAGE(page_table_l4), %eax
	movl %eax, %cr3

	lgdt V2P_IMAGE(gdtr)

	/* Enable PAE */
	mov %cr4, %eax
	bts $5, %eax
	mov %eax, %cr4

	/* Enable EFER.LME and EFER.SCE */
	mov $0xc0000080, %ecx
	rdmsr
	or $((1<<8) | (1<<0)), %eax
	wrmsr

	/* Enable paging, which enables long mode */
	mov %cr0, %eax
	bts $31, %eax
	mov %eax, %cr0

	movw $0x0f41, 0xb8000
	/* long jump */
	ljmp $8, $V2P_IMAGE(1f)

.code64
1:
code64:
	movw $0x0f42, 0xb8002
	movabs $1f, %rax
	jmp *%rax /* Switch to upper-half addresses */
1:
	/* Switch GDTR and rsp to upper-half addresses, load segment regs */
	movq $gdt, gdtr+2
	lgdt gdtr
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs
	movabs $(idle_task_stack + PAGE_SIZE - 0xb0), %rsp

	/* Unmap low memory */
	movq $0, page_table_l3
	movq $0, page_table_l4
	invlpg 0
	invlpg 0x200000

	/* Zero frame pointer and call kernel_start */
	xor %ebp, %ebp
	mov %esi, %edi
	mov %ebx, %esi
	mov $KERNEL_VIRT_ADDR, %rax
	add %rax, %rsi
	call kernel_start
	cli
	hlt

.data
.align 8
.global gdt
gdt:
	.long 0, 0		/* null */
	.long 0, 0x00209800	/* kernel code */
	.long 0, 0x00009200	/* kernel stack */
	.long 0, 0, 0, 0	/* TSS */
	.long 0, 0x0000f200	/* user stack */
	.long 0, 0x0020f800	/* user code */
gdtr:
	.word . - gdt - 1
	.long V2P_IMAGE(gdt)
	.long 0
