#include <arch.h>
#include <video.h>
#include <mm.h>
#include <syms.h>
#include <kernel.h>
#include <irq.h>


void f(void **a)
{
	int i;
	for (i=0; i<1000; i++)
	{
		a[i] = alloc_page();
		if (!a[i]) {printk("i=%d\n", i); BUG();}
	}
}

void g(void **a)
{
	int i;
	for (i=0; i<1000; i++)
	{
		free_page(a[i]);
	}
}

void test_mm()
{
	void **a, **b, **c;
	unsigned long f1, f2, f3;
	a = alloc_page();
	b = alloc_page();
	c = alloc_page();
	f1 = get_free_mem();
	f(a);
	f(b);
	f(c);
	f2 = get_free_mem();
	g(a);
	g(b);
	g(c);
	f3 = get_free_mem();
	printk("f1=%u\nf2=%u\nf3=%u\n", f1, f2, f3);
	free_page(a);
	free_page(b);
	free_page(c);
}

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
	printk("Memory used: %u bytes.\n", get_used_mem());

	/* idle loop */
	while (1) halt();
}
