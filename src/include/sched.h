#ifndef SCHED_H
#define SCHED_H

#define FLAG_NEED_RESCHED		0
#define FLAG_IN_SYSCALL			1

#ifndef ASM

#include <stddef.h>
#include <kernel.h>

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

#endif

#endif
