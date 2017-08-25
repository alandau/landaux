#ifndef SCHED_H
#define SCHED_H

#define FLAG_NEED_RESCHED		0

#ifndef ASM

#include <stddef.h>
#include <kernel.h>
#include <list.h>
#include <process.h>

extern volatile u64 global_flags;

#define set_need_resched()	set_global_flag(FLAG_NEED_RESCHED)
#define clear_need_resched()	clear_global_flag(FLAG_NEED_RESCHED)
#define need_resched()		get_global_flag(FLAG_NEED_RESCHED)

static inline void set_global_flag(int flag)
{
	if (flag < 0 || flag >= 64) BUG();
	__asm__ __volatile__ ("or %1, %0" : "=m"(global_flags) : "r"(1UL<<flag));		// this must be atomic
}

static inline void clear_global_flag(int flag)
{
	if (flag < 0 || flag >= 64) BUG();
	__asm__ __volatile__ ("and %1, %0" : "=m"(global_flags) : "r"(~(1UL<<flag)));	// this must be atomic
}

static inline int get_global_flag(int flag)
{
	if (flag < 0 || flag >= 64) BUG();
	return global_flags & (1UL << flag);
}


void schedule(void);

typedef struct {
	list_t head;
} waitqueue_t;

#define WAITQUEUE_INIT(name) { LIST_INIT(name.head) }

static inline void waitqueue_sleep(waitqueue_t *q) {
	BUG_ON(current->state != TASK_RUNNING);
	u64 flags = save_flags_irq();
	// schedule() will remove from runqueue
	current->state = TASK_INTERRUPTIBLE;
	list_add(&q->head, &current->waitq);
	set_need_resched();
	restore_flags(flags);
}

static inline void waitqueue_wakeup(waitqueue_t *q) {
	u64 flags = save_flags_irq();
	list_t *iter;
	list_for_each(&q->head, iter) {
		task_t *p = list_get(iter, task_t, waitq);
		BUG_ON(p->state != TASK_INTERRUPTIBLE && p->state != TASK_UNINTERRUPTIBLE);
		list_del(&p->waitq);
		p->state = TASK_RUNNING;
		list_add(&idle->running, &p->running);
	}
	restore_flags(flags);
}

#endif

#endif
