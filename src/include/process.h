#ifndef PROCESS_H
#define PROCESS_H

#include <arch.h>
#include <list.h>

#define TASK_RUNNING			0
#define TASK_INTERRUPTIBLE		1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE				3

typedef struct {
	u32 esp;
	u32 eip;
} switch_regs_t;

typedef struct task_struct
{
	int pid;
	u32 timeslice;
	int need_resched;
	int state;
	int exit_code;

	list_t tasks;
	list_t running;

	void *mm;
	switch_regs_t regs;
} task_t;

typedef union {
	task_t task;
	u32 stack[1024];
} task_stack_t;

#define current ((task_t *)(get_esp() & ~(sizeof(task_stack_t)-1)))

extern task_t *idle;

#define for_each_task(p) list_for_each(idle->processes, task_t, tasks)

typedef struct {
	u16 back_link, __back_link;
	u32 esp0;
	u16 ss0, __ss0;
	u32 esp1;
	u16 ss1, __ss1;
	u32 esp2;
	u16 ss2, __ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax, ecx, edx, ebx, esp, ebp, esi, edi;
	u16 es, __es;
	u16 cs, __cs;
	u16 ss, __ss;
	u16 ds, __ds;
	u16 fs, __fs;
	u16 gs, __gs;
	u16 ldt, __ldt;
	u16 trap;
	u16 io_map_base;
} tss_t __attribute__((packed));

void init_idle(void);
void init_tss(void);
int fork(void);


#endif
