#include <kernel.h>
#include <sched.h>
#include <process.h>
#include <mm.h>

volatile u64 global_flags;
extern volatile u64 jiffies;

typedef struct {
	u64 time;
	task_t *task;
	list_t sleepq;
} timer_waitq_t;

timer_waitq_t timerq;

void init_scheduler(void) {
	list_init(&timerq.sleepq);
}

void scheduler_tick(void)
{
	u64 now = jiffies;
	list_t *it;
	list_for_each(&timerq.sleepq, it) {
		timer_waitq_t *t = list_get(it, timer_waitq_t, sleepq);;
		if (now >= t->time) {
			list_del(it);
			list_add(&idle->running, &t->task->running);
			t->task->state = TASK_RUNNING;
			kfree(t);
			if (current == idle) {
				set_need_resched();
			}
		}
	}
	task_t *p = current;
	if (!--p->timeslice)
	{
		set_need_resched();
		p->timeslice = 20;
	}
}

extern tss_t tss;

void context_switch_tail(task_t *prev, task_t *next)
{
	extern u64 swapgs_kernel_rsp;
	tss.rsp0 = (u64)next + sizeof(task_stack_t);
	swapgs_kernel_rsp = tss.rsp0;
	if (prev->state == TASK_ZOMBIE) {
		free_task(prev);
	}
}

#define context_switch(prev, next, flags) do {		\
	__asm__ __volatile__ (				\
		"push %5\n\t"				\
		"push %%rbp\n\t"			\
		"push %%rbx\n\t"			\
		"push %%r12\n\t"			\
		"push %%r13\n\t"			\
		"push %%r14\n\t"			\
		"push %%r15\n\t"			\
		"mov %%rsp, %0\n\t"			\
		"mov %2, %%rsp\n\t"			\
		"movq $1f, %1\n\t"			\
		"mov %4, %%cr3\n\t"			\
		"push %3\n\t"				\
		"jmp context_switch_tail\n\t"		\
		"1:\n\t"				\
		"pop %%r15\n\t"				\
		"pop %%r14\n\t"				\
		"pop %%r13\n\t"				\
		"pop %%r12\n\t"				\
		"pop %%rbx\n\t"				\
		"pop %%rbp\n\t"				\
		"popf\n\t"				\
		: "=m"(prev->regs.rsp), "=m"(prev->regs.rip)	\
		: "m"(next->regs.rsp), "m"(next->regs.rip), "a"(get_task_cr3(&next->mm)), \
		  "d"(flags), "D"(prev), "S"(next)	\
		: "memory"				\
		);					\
	} while (0)

void schedule(void)
{
	u64 flags = save_flags_irq();
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

long sys_usleep(long us) {
	u64 when = jiffies + (us * HZ + 999999 ) / 1000000;
	list_del(&current->running);
	current->state = TASK_INTERRUPTIBLE;
	timer_waitq_t *q = kmalloc(sizeof(timer_waitq_t));
	q->time = when;
	q->task = current;
	list_init(&q->sleepq);
	list_add(&timerq.sleepq, &q->sleepq);
	set_need_resched();
	return 0;
}
