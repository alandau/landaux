#ifndef PROCESS_H
#define PROCESS_H

#include <arch.h>
#include <list.h>
#include <mm.h>

#define TASK_RUNNING			0
#define TASK_INTERRUPTIBLE		1
#define TASK_UNINTERRUPTIBLE		2
#define TASK_ZOMBIE			3

typedef struct {
	u64 rsp;
	u64 rip;
} switch_regs_t;

typedef struct task_struct
{
	int pid;
	u32 timeslice;
	int state;
	int exit_code;

	list_t tasks;
	list_t running;
	list_t waitq;

	mm_t mm;
	switch_regs_t regs;
	file_t *pdt[10];
} task_t;

typedef union {
	task_t task;
	u32 stack[1024];
} task_stack_t;
typedef int BUG_task_stack_t_is_not_page_sized[sizeof(task_stack_t) == PAGE_SIZE ? 1 : -1];

#define current ((task_t *)(get_rsp() & ~(sizeof(task_stack_t)-1)))

extern task_t *idle;

#define for_each_task(p) list_for_each(idle->processes, task_t, tasks)

typedef struct {
	u32 reserved1;
	u64 rsp0;
	u64 rsp1;
	u64 rsp2;
	u64 reserved2;
	u64 ist[7];
	u64 reserved3;
	u16 reserved4;
	u16 io_map_base;
} __attribute__((packed)) tss_t;

void init_idle(void);
void init_tss(void);
int kernel_thread(void (*fn)(void *data), void *data);
int kernel_exec(const char *path);
void free_task(task_t *p);


#endif
