#include <arch.h>
#include <video.h>
#include <mm.h>
#include <syms.h>
#include <kernel.h>
#include <irq.h>
#include <process.h>
#include <sched.h>
#include <block.h>


void f(void **a, int count)
{
	int i;
	for (i=0; i<1000*count; i++)
	{
		a[i] = alloc_page(MAP_READWRITE);
		if (!a[i]) {printk("i=%d\n", i); BUG();}
	}
}

void g(void **a, int count)
{
	int i;
	for (i=0; i<1000*count; i++)
	{
		free_page(a[i]);
	}
}

void test_mm(void)
{
	void **a;
	u32 f1, f2, f3;
	f1 = get_free_mem();
	a = alloc_pages(3, MAP_READWRITE);
//	f(a, 3);
	f2 = get_free_mem();
//	g(a, 3);
	free_pages(a, 3);
	f3 = get_free_mem();
	printk("f1=%u\nf2=%u\nf3=%u\n", f1, f2, f3);
}

int sys_ni_syscall(void)
{
	return -1;
}

static inline int user_fork(void)
{
	int ret;
	__asm__ __volatile__ ("int $0x30" : "=a"(ret) : "0"(1));
	return ret;
}

/* This is the C entry point of the kernel. It runs with interrupts disabled */
void kernel_start(void)
{
	init_mm();
	init_video();
	init_pic();
	init_pit();
	init_idle();
	init_tss();
	void do_keyboard(void *);
	void do_timer(void *);
	register_irq(0, do_timer, NULL);
	register_irq(1, do_keyboard, NULL);
	sti();
	printk("Found %u MB of memory.\n", get_mem_size()/1024/1024);
	printk("Memory used: %u bytes.\n", get_used_mem());
	
	move_to_user_mode();
	if (user_fork() == 0)
	{
		// child
		char str[]="in init\n";
		int tmp;
		//printk(str);
		__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(0), "b"(str));
		__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(2), "b"(str));
		while (1);
	}
	
	// parent is the idle task
	{
		const char *str="in idle\n";
		int tmp;
		__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(0), "b"(str));
	}
	
	/* idle loop */
	while (1) halt();
}
