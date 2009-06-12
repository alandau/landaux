#include <process.h>
#include <list.h>
#include <mm.h>
#include <kernel.h>
#include <string.h>
#include <sched.h>
#include <fs.h>

task_stack_t idle_task_stack __attribute__ ((aligned (PAGE_SIZE)));
tss_t tss __attribute__ ((aligned (PAGE_SIZE)));
task_t *idle;
static int next_free_pid;


void init_idle(void)
{
	idle = &idle_task_stack.task;
	memset(idle, 0, sizeof(task_t));
	extern pte_t page_dir[];
	idle->mm.page_dir = page_dir;
	list_init(&idle->mm.vm_areas);
	idle->pid = 0;
	idle->state = TASK_RUNNING;
	idle->timeslice = 1;
	list_init(&idle->tasks);
	list_init(&idle->running);
	set_need_resched();
	next_free_pid = 1;
}

void init_tss(void)
{
	memset(&tss, 0, sizeof(tss));
	tss.ss0 = KERNEL_DS;
	tss.esp0 = (u32)(&idle_task_stack + 1);
	__asm__ __volatile__ ("ltrw %%ax" : /* no output */ : "a" (TSS_SELECTOR));
}

int sys_fork(void)
{
#if 1
	extern char ret_from_interrupt[];

	task_t *p = alloc_page(MAP_READWRITE);
	*p = *current;
	p->pid = next_free_pid++;
	BUG_ON(p->pid < 0);		// overflow
	list_add(&current->tasks, &p->tasks);
	list_add(&current->running, &p->running);
	int ret = 0;
	if ((ret = clone_mm(p)) < 0)
		goto out;
	regs_t *parent_regs = (regs_t *)((task_stack_t *)current + 1) - 1;
	regs_t *child_regs = (regs_t *)((task_stack_t *)p + 1) - 1;
	*child_regs = *parent_regs;
	child_regs->eax = 0;	// son returns 0
	p->regs.eip = (u32)ret_from_interrupt;
	p->regs.esp = (u32)child_regs;
	return p->pid;
out:
	free_page(p);
	return ret;
#else
	return 0;
#endif
}

int kernel_thread(void (*fn)(void *data), void *data)
{
	task_t *p = alloc_page(MAP_READWRITE);
	*p = *idle;
	p->pid = next_free_pid++;
	BUG_ON(p->pid < 0);		// overflow
	list_add(&idle->tasks, &p->tasks);
	list_add(&idle->running, &p->running);
	list_init(&p->mm.vm_areas);
	p->timeslice = 10;
	p->regs.eip = (u32)fn;
	p->regs.esp = (u32)((char *)p + sizeof(task_stack_t));
	*(u32 *)(p->regs.esp -= 4) = (u32)data;
	*(u32 *)(p->regs.esp -= 4) = 0;	//guard against function exit
	return p->pid;
}

int sys_exit(void)
{
#if 0
	free_mm(&current->mm);
	list_del(&current->tasks);
	list_del(&current->running);
	free_pages(current, sizeof(task_stack_t)/PAGE_SIZE);
#endif
	return 0;
}

#define CODE_START	0x100000	/* 1MB */
#define STACK_START	KERNEL_VIRT_ADDR
#define STACK_SIZE	PAGE_SIZE

int do_exec(const char *path)
{
	int ret;
	printk("do_exec: path=<%s>\n", path);
	file_t *f = vfs_open(path, O_RDONLY);
	if (IS_ERR(f)) {
		printk("do_exec: Can't open file '%s' (%d)\n", path, PTR_ERR(f));
		return PTR_ERR(f);
	}
	u32 frame = alloc_phys_page();
	void *p = (void *)CODE_START;
	map_user_page(frame, (u32)p, 0 /* should be PTE_EXEC, but x86 sucks */);
	ret = vfs_read(f, p, PAGE_SIZE);
	if (ret < 0) {
		printk("do_exec: Can't read from file '%s' (%d)\n", path, ret);
		vfs_close(f);
		goto out;
	}
	vfs_close(f);

	/* setup stack */
	u32 stack = alloc_phys_page();
	p = (void *)(STACK_START - STACK_SIZE);
	map_user_page(stack, (u32)p, PTE_WRITE);

	/* setup mm areas */
	ret = mm_add_area(CODE_START, PAGE_SIZE, VM_AREA_READ|VM_AREA_EXEC);
	if (ret < 0)
		goto out2;
	ret = mm_add_area(STACK_START - STACK_SIZE, STACK_SIZE, VM_AREA_READ|VM_AREA_WRITE);
	if (ret < 0)
		goto out2;

	regs_t *r = (regs_t *)((task_stack_t *)current + 1) - 1;
	r->eax = r->ebx = r->ecx = r->edx = r->esi = r->edi = r->ebp = 0;
	r->cs = USER_CS;
	r->ds = r->es = r->fs = r->gs = r->ss = USER_DS;
	r->eip = CODE_START;
	r->esp = STACK_START;
	r->eflags = (1<<9) | (1<<1);	// 9 is IF, 1 is always on
	printk("do_exec: success\n");
	return 0;
out2:
	free_phys_page(stack);
out:
	free_phys_page(frame);
	return ret;
}

int sys_exec(const char *path)
{
#if 0
#define CODE_START (1024*1024)	// 1MB
#define STACK_START KERNEL_VIRT_ADDR
	u32 code_size = end_process_code - process_code;
	u32 code_pages = (code_size + PAGE_SIZE-1) >> PAGE_SHIFT;
	int i;
	// map code
	for (i=0; i<code_pages; i++)
	{
		u32 frame = alloc_phys_page();
		if (frame == 0) return -1;
		if (ioremap(frame << PAGE_SHIFT, PAGE_SIZE, CODE_START+(i<<PAGE_SHIFT), MAP_USERSPACE) == NULL)
			return -1;
	}
	// map stack
	u32 frame = alloc_phys_page();
	if (frame == 0) return -1;
	if (ioremap(frame << PAGE_SHIFT, PAGE_SIZE, STACK_START-(1<<PAGE_SHIFT), MAP_USERSPACE | MAP_READWRITE) == NULL)
		return -1;

	// copy code and setup usermode registers
	memcpy((void *)CODE_START, process_code, code_size);
	regs_t *regs = (regs_t *)((task_stack_t *)current + 1) - 1;
	regs->eflags = (1<<9) | (1<<1);	// 9 is IF, 1 is always on
	regs->eip = CODE_START;
	regs->esp = STACK_START;
#endif
	return do_exec(path);
}

int kernel_exec(const char *path)
{
	int ret;
	/* setup the stack as if ss and esp were pushed and call exec() via a syscall */
	__asm__ __volatile__ (
		"orl $" STR(PAGE_SIZE-1) ", %%esp\n\t"
		"subl $7, %%esp\n\t"
		"int $0x30"
		: "=a" (ret) : "0" (2), "d" (path));
	return ret;
}
