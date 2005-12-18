#include <sched.h>
#include <process.h>
#include <video.h>

void scheduler_tick(void)
{
	task_t *p = current;
	if (--p->timeslice)
	{
		p->need_resched = 1;
		p->timeslice = 100;
	}
}

#define context_switch(prev, next)								\
	__asm__ __volatile__ (										\
		"pusha\n\t"												\
		"movl %%esp, %0\n\t"									\
		"movl %2, %%esp\n\t"									\
		"movl $1f, %1\n\t"										\
		"pushl %3\n\t"											\
		"ret\n\t"												\
		"1:\n\t"												\
		"popa\n\t"												\
		: "=m"(prev->regs.esp), "=m"(prev->regs.eip)			\
		: "m"(next->regs.esp), "m"(next->regs.eip)				\
		: "%eax", "%ebx", "%ecx", "%edx", "ebp"					\
		)

void schedule(void)
{
	u32 flags = save_flags_irq();
	task_t *prev = current;
	task_t *next = list_get(prev->running.next, task_t, running);
	if (next == idle)
	{
		task_t *next_next = list_get(next->running.next, task_t, running);
		if (next_next != idle) next = next_next;
	}
	prev->need_resched = 0;
	printk("next=%d\n", next->pid);
	if (next != prev) context_switch(prev, next);
	restore_flags(flags);
}
