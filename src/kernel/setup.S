#include <multiboot.h>
#include <mem.h>

.text

.align 4
/* Multiboot header */
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.global _start
_start:
	/* %eax holds MULTIBOOT_BOOTLOADER_MAGIC, %ebx points to multiboot_info_t */

	/* Initialize stack to be able to initialize %eflags */
	movl $(V2P(idle_task_stack) + PAGE_SIZE), %esp

	/* Initialize %eflags */
	pushl $0
	popfl

	/* Put multiboot magic aside */
	movl %eax, %esi

	/* Identity map first 4MB (also map them to KERNEL_VIRT_ADDR) */
	cld
	movl $V2P(kernel_page_table), %edi
	movl $(PTE_PRESENT | PTE_WRITE), %eax
	movl $1024, %ecx
1:	stosl
	addl $PAGE_SIZE, %eax
	loop 1b
	movl $(V2P(kernel_page_table) + PTE_PRESENT + PTE_WRITE), V2P(page_dir)
	movl $(V2P(kernel_page_table) + PTE_PRESENT + PTE_WRITE), V2P(page_dir) + KERNEL_VIRT_ADDR/0x400000 * 4

	/* Enable paging */
	movl $V2P(page_dir), %eax
	movl %eax, %cr3
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0

	leal 1f, %eax
	jmpl *%eax

/* Paging enabled */
1:
	/* Update stack pointer and unmap the first 4MB */
	addl $KERNEL_VIRT_ADDR, %esp
	movl $0, page_dir
	invlpg 0

	/* Final preparations and call main() */
	xorl %ebp, %ebp		/* Terminate stack frame */
	addl $KERNEL_VIRT_ADDR, %ebx
	pushl %ebx
	pushl %esi
	call kernel_start

	/* kernel_start() should not return... */
1:	hlt
	jmp 1b
