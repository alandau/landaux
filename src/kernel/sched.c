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

/*
 * context_switch_tail() is regparm(2), which means that prev is in %eax,
 * and next is in %edx.
 */
void __attribute__((regparm(2))) context_switch_tail(task_t *prev, task_t *next)
{
	tss.esp0 = (u32)next + sizeof(task_stack_t);
	if (prev->state == TASK_ZOMBIE) {
		free_task(prev);
	}
}

#define context_switch(prev, next, flags) do {		\
	__asm__ __volatile__ (				\
		"pushl %%ebp\n\t"			\
		"pushl %%ebx\n\t"			\
		"movl %%esp, %0\n\t"			\
		"movl %2, %%esp\n\t"			\
		"movl $1f, %1\n\t"			\
		"movl %4, %%cr3\n\t"			\
		"pushl %3\n\t"				\
		"jmp context_switch_tail\n\t"		\
		"1:\n\t"				\
		"popl %%ebx\n\t"			\
		"popl %%ebp\n\t"			\
		"pushl %5\n\t"				\
		"popfl\n\t"				\
		: "=m"(prev->regs.esp), "=m"(prev->regs.eip)	\
		: "S"(next->regs.esp), "D"(next->regs.eip), "c"(get_task_cr3(&next->mm)), \
		  "m"(flags), "a"(prev), "d"(next)	\
		: "memory"				\
		);					\
	} while (0)

void schedule(void)
{
	u32 flags = save_flags_irq();
	task_t *prev = current;
	task_t *next = list_get(prev->running.next, task_t, running);
	if (prev->state == TASK_ZOMBIE) {
		list_del(&prev->running);
	}
	if (next == idle) {
		task_t *next_next = list_get(next->running.next, task_t, running);
		if (next_next != idle)
			next = next_next;
	}
	clear_need_resched();
	if (next != prev) {
		context_switch(prev, next, flags);	/* context_switch restores flags */
		/*
		 * Everything after context_switch() is executed in the context
		 * of next. The stack also belongs to next, thus all local variables
		 * may contain different values than before context_switch().
		 * All further processing on the old values of prev and next should be
		 * done in context_switch_tail().
		 */
	} else
		restore_flags(flags);
}
