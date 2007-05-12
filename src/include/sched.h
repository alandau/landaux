#ifndef SCHED_H
#define SCHED_H

#include <stddef.h>
#include <kernel.h>

extern volatile u32 global_flags;

#define FLAG_NEED_RESCHED		0
#define FLAG_IN_SYSCALL			1

#define set_need_resched()		set_global_flag(FLAG_NEED_RESCHED)
#define clear_need_resched()	clear_global_flag(FLAG_NEED_RESCHED)

static inline void set_global_flag(int flag)
{
	if (flag < 0 || flag >= 32) BUG();
	__asm__ __volatile__ ("orl %1, %0" : "=m"(global_flags) : "r"(1<<flag));		// this must be atomic
}

static inline void clear_global_flag(int flag)
{
	if (flag < 0 || flag >= 32) BUG();
	__asm__ __volatile__ ("andl %1, %0" : "=m"(global_flags) : "r"(~(1<<flag)));	// this must be atomic
}

void schedule(void);

#endif
