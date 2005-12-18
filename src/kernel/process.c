#include <process.h>
#include <list.h>
#include <mm.h>
#include <kernel.h>
#include <string.h>

task_stack_t idle_task_stack __attribute__ ((aligned (PAGE_SIZE)));
task_t *idle;
static int next_free_pid;

tss_t tss;

void init_idle(void)
{
	extern char page_dir[];
	idle = &idle_task_stack.task;
	if (current != idle) BUG();
	idle->mm = page_dir;
	idle->pid = 0;
	list_init(&idle->tasks);
	list_init(&idle->running);
	next_free_pid = 1;
}

void init_tss(void)
{
	memset(&tss, 0, sizeof(tss));
	tss.ss0 = KERNEL_DS;
	tss.esp0 = (u32)(&idle_task_stack + 1);

	extern char tss_des[];
	u32 tss_addr = virt_to_phys(&tss);
	*(u16 *)(tss_des+2) = tss_addr & 0xFFFF;
	*(u8 *)(tss_des+4) = (tss_addr >> 16) & 0xFF;
	*(u8 *)(tss_des+7) = tss_addr >> 24;
	__asm__ __volatile__ ("ltrw %%ax" : /* no output */ : "a" (TSS_SELECTOR));
}

int fork(void)
{
	extern char ret_from_sys_call[];
	printk("in fork()\n");
	return 0;
//	regs_t *r = (regs_t *)((task_stack_t *)current + 1) - 1;
//	dump_regs(r);
//	return 0;

	task_t *p = alloc_pages(sizeof(task_stack_t) / PAGE_SIZE);
	*p = *current;
	p->pid = next_free_pid++;
	p->need_resched = 0;
	p->state = TASK_RUNNING;
	list_init(&p->tasks);
	list_init(&p->running);
	list_add(&current->tasks, &p->tasks);
	list_add(&current->running, &p->running);
//	p->mm = clone_mm(current->mm);
//	if (!p->mm) return -1;
	regs_t *parent_regs = (regs_t *)((task_stack_t *)current + 1) - 1;
	regs_t *child_regs = (regs_t *)((task_stack_t *)p + 1) - 1;
	*child_regs = *parent_regs;
	child_regs->eax = 0;	// son returns 0
	p->regs.eip = (u32)ret_from_sys_call;
	p->regs.esp = (u32)child_regs;
	dump_regs(child_regs);
	return p->pid;
}
