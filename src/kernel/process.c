#include <process.h>
#include <list.h>
#include <mm.h>
#include <kernel.h>
#include <string.h>
#include <sched.h>

task_stack_t idle_task_stack __attribute__ ((aligned (PAGE_SIZE)));
static task_stack_t *real_idle_stack;
task_t *idle;
static int next_free_pid;

tss_t tss;

void init_idle(void)
{
	real_idle_stack = alloc_pages(sizeof(task_stack_t) / PAGE_SIZE, MAP_READWRITE);
	if (!real_idle_stack) BUG();
	memcpy(real_idle_stack, &idle_task_stack, sizeof(task_stack_t));
	__asm__ __volatile__ ("addl %0, %%esp"
			: /* no outputs */
			: "g"((char *)real_idle_stack - (char *)&idle_task_stack)
			: "%esp");
	idle = &real_idle_stack->task;
	// current points to idle_task_stack. we initialize current->mm since virt_to_phys needs it (called in init_tss)
	init_task_mm(&idle->mm);
	current->mm = idle->mm;
	idle->pid = 0;
	idle->timeslice = 1;
	list_init(&idle->tasks);
	list_init(&idle->running);
	set_need_resched();
	next_free_pid = 1;
}

void init_tss(void)
{
	memset(&tss, 0, sizeof(tss));
	tss.ss0 = tss.ss = KERNEL_DS;
	tss.esp0 = (u32)(real_idle_stack + 1);
	tss.ldt = LDT_SELECTOR;
	tss.cr3 = get_task_cr3(&current->mm);
	tss.io_map_base = sizeof(tss_t);

	extern char tss_des[];
	u32 tss_addr = (u32)&tss;//virt_to_phys(&tss);
	printk("%0x", tss_addr);
//	halt();
	*(u16 *)(tss_des+2) = tss_addr & 0xFFFF;
	*(u8 *)(tss_des+4) = (tss_addr >> 16) & 0xFF;
	*(u8 *)(tss_des+7) = tss_addr >> 24;
	extern void setup_idtr(void);
	setup_idtr();
	printk("haha\n");
//	asm ("int $0x0");
//	*(char*)0 = 1;

//	halt();
	__asm__ __volatile__ ("ltrw %%ax" : /* no output */ : "a" (TSS_SELECTOR));
//	halt();
}

int sys_fork(void)
{
	extern char ret_from_sys_call[];

	task_t *p = alloc_pages(sizeof(task_stack_t) / PAGE_SIZE, MAP_READWRITE);
	*p = *current;
	p->pid = next_free_pid++;
	if (p->pid < 0) BUG();		// overflow
	p->state = TASK_RUNNING;
	list_init(&p->tasks);
	list_init(&p->running);
	list_add(&current->tasks, &p->tasks);
	list_add(&current->running, &p->running);
	if (!clone_mm(&p->mm)) return -1;
	regs_t *parent_regs = (regs_t *)((task_stack_t *)current + 1) - 1;
	regs_t *child_regs = (regs_t *)((task_stack_t *)p + 1) - 1;
	*child_regs = *parent_regs;
	child_regs->eax = 0;	// son returns 0
	p->regs.eip = (u32)ret_from_sys_call;
	p->regs.esp = (u32)child_regs;
	return p->pid;
}

int kernel_thread(void (*fn)(void *data), void *data)
{
	task_t *p = alloc_pages(sizeof(task_stack_t) / PAGE_SIZE, MAP_READWRITE);
	*p = *idle;
	p->pid = next_free_pid++;
	if (p->pid < 0) BUG();		// overflow
	p->state = TASK_RUNNING;
	list_init(&p->tasks);
	list_init(&p->running);
	list_add(&idle->tasks, &p->tasks);
	list_add(&idle->running, &p->running);
	p->timeslice = 10;
	regs_t *parent_regs = (regs_t *)((task_stack_t *)idle + 1) - 1;
	regs_t *child_regs = (regs_t *)((task_stack_t *)p + 1) - 1;
	*child_regs = *parent_regs;
	p->regs.eip = (u32)fn;
	p->regs.esp = (u32)((char *)p + sizeof(task_stack_t));
	*(u32 *)(p->regs.esp -= 4) = (u32)data;
	*(u32 *)(p->regs.esp -= 4) = 0;	//guard against function exit
	return p->pid;
}

static void process_code(void)
{
	int tmp;
	char buf[50];
	buf[0]='@';buf[1]='!';buf[2]='@';buf[3]='\n';buf[4]=0;
	__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(0), "b"(buf));
	__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(1));
	if (tmp)
	{
		while (1)
		{
			buf[2]='1';
			__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(0), "b"(buf));
		}
	}
	else
	{
		while (1)
		{
			buf[2]='2';
			__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(0), "b"(buf));
		}
	}
}

static void end_process_code(void) {}

int sys_exec(void)
{
#define CODE_START (1024*1024)	// 1MB
#define STACK_START KERNEL_VIRT_ADDRESS
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
	return 0;
}

int sys_exit(void)
{
	free_mm(&current->mm);
	list_del(&current->tasks);
	list_del(&current->running);
	free_pages(current, sizeof(task_stack_t)/PAGE_SIZE);
	return 0;
}
