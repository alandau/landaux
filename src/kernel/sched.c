#include <kernel.h>
#include <sched.h>
#include <process.h>
#include <mm.h>

volatile u32 global_flags;

void scheduler_tick(void)
{
	u32 flags;
	asm ("pushfl;popl %0": "=r"(flags));
	if (flags & (1<<14)) printk("NT is set\nNT is set\nNT is set\n");
	task_t *p = current;
	if (!--p->timeslice)
	{
		set_need_resched();
		p->timeslice = 20;
	}
}

extern tss_t tss;

#define context_switch(prev, next) do {							\
	tss.esp0 = (u32)next + sizeof(task_stack_t);				\
	__asm__ __volatile__ (										\
		"pusha\n\t"												\
		"movl %%esp, %0\n\t"									\
		"movl %2, %%esp\n\t"									\
		"movl $1f, %1\n\t"										\
		"movl %4, %%cr3\n\t"									\
		"pushfl\n\t"											\
		"pushl %%cs\n\t"										\
		"pushl %3\n\t"											\
		"iret\n\t"												\
		"1:\n\t"												\
		"popa\n\t"												\
		: "=m"(prev->regs.esp), "=m"(prev->regs.eip)			\
		: "m"(next->regs.esp), "m"(next->regs.eip), "a"(get_task_cr3(&next->mm))				\
		: "%ebx", "%ecx", "%edx"								\
		);														\
	} while (0)

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
	clear_need_resched();
	if (next != prev) context_switch(prev, next);
	restore_flags(flags);
}
