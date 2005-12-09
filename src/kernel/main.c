#include <arch.h>
#include <video.h>
#include <mm.h>
#include <syms.h>
#include <kernel.h>
#include <irq.h>

/* This is the C entry point of the kernel. It runs with interrupts disabled */
void kernel_start(void)
{
	init_video();
	init_mm();
	init_pic();
	init_pit();
	void do_keyboard(void *);
	void do_timer(void *);
	register_irq(0, do_timer, NULL);
	register_irq(1, do_keyboard, NULL);
	sti();
	
	printk("0x%x\n", alloc_page());
	printk("0x%x\n", alloc_page());
	printk("0x%x\n", alloc_page());
	/* idle loop */
	while (1) halt();
}
