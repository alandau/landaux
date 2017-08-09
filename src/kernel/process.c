#include <process.h>
#include <list.h>
#include <mm.h>
#include <kernel.h>
#include <string.h>
#include <sched.h>
#include <fs.h>
#include <elf.h>

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
	if (current->pid == 1) {
		printk("BUG: PID 1 exiting\n");
		BUG();
	}
	current->state = TASK_ZOMBIE;
	free_mm();
	schedule();	/* will call free_task() after a context switch */
	BUG();
	return 0;
}

void free_task(task_t *p)
{
	list_del(&p->tasks);
	free_page(p->mm.page_dir);
	free_page(p);
}

#define STACK_START	KERNEL_VIRT_ADDR
#define STACK_SIZE	PAGE_SIZE

static u32 elf_flags_to_mmap_prot(Elf32_Word flags)
{
	u32 ret = 0;
	if (flags & PF_R) ret |= PROT_READ;
	if (flags & PF_W) ret |= PROT_WRITE;
	if (flags & PF_X) ret |= PROT_EXEC;
	return ret;
}

static int do_exec_elf(const char *path)
{
	int ret;
	printk("do_exec_elf: path=<%s>\n", path);
	file_t *f = vfs_open(path, O_RDONLY);
	if (IS_ERR(f)) {
		printk("do_exec_elf: Can't open file '%s' (%d)\n", path, PTR_ERR(f));
		return PTR_ERR(f);
	}
	Elf_Ehdr ehdr;
	ret = vfs_read(f, &ehdr, sizeof(ehdr));
	if (ret < 0) {
		vfs_close(f);
		printk("do_exec_elf: Can't read ELF header '%s' (%d)\n", path, ret);
		return ret;
	}
	if (ret != sizeof(ehdr)) {
		vfs_close(f);
		printk("do_exec_elf: Can't read full ELF header '%s' (%d)\n", path, ret);
		return -ENOEXEC;
	}
	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 || ehdr.e_ident[EI_CLASS] != ELFCLASS32 ||
			ehdr.e_ident[EI_DATA] != ELFDATA2LSB || ehdr.e_ident[EI_VERSION] != EV_CURRENT) {
		vfs_close(f);
		printk("do_exec_elf: Not an ELF file '%s'\n", path);
		return -ENOEXEC;
	}
	if (ehdr.e_type != ET_EXEC || ehdr.e_machine != EM_386 || ehdr.e_version != EV_CURRENT || ehdr.e_phoff == 0) {
		vfs_close(f);
		printk("do_exec_elf: Not an x86 executable ELF file '%s'\n", path);
		return -ENOEXEC;
	}
	int i;
	Elf32_Word stack_flags = PF_R | PF_W | PF_X;
	for (i = 0; i < ehdr.e_phnum; i++) {
		Elf_Phdr phdr;
		ret = vfs_lseek(f, ehdr.e_phoff + i*sizeof(phdr), SEEK_SET);
		if (ret < 0) {
			vfs_close(f);
			printk("do_exec_elf: Can't seek past EOF '%s' (%d)\n", path, ret);
			return ret;
		}
		ret = vfs_read(f, &phdr, sizeof(phdr));
		if (ret < 0) {
			vfs_close(f);
			printk("do_exec_elf: Can't read segment %d '%s' (%d)\n", i, path, ret);
			return ret;
		}
		if (ret != sizeof(phdr)) {
			vfs_close(f);
			printk("do_exec_elf: Can't read full segment %d '%s' (%d)\n", i, path, ret);
			return -ENOEXEC;
		}
		switch (phdr.p_type) {
		case PT_NULL:
		case PT_NOTE:
			/* ignore */
			break;
		case PT_LOAD:
			/*
			 * Map file offset phdr.p_offset of size phdr.p_filesz to
			 * virtual address phdr.p_vaddr of size phdr.p_memsz.
			 * p_filesz may be less (but not greater) than p_memsz, in which
			 * case, the spare memory area should be zero-filled.
			 * p_vaddr and p_offset are not neccessarily page-aligned, but
			 * p_vaddr = p_offset (mod PAGE_SIZE).
			 */
			if (phdr.p_filesz > phdr.p_memsz) {
				vfs_close(f);
				printk("do_exec_elf: File size > mem size in segment %d '%s'\n", i, path);
				return -ENOEXEC;
			}
			u32 offset_in_page = phdr.p_offset & ~PAGE_MASK;
			if ((phdr.p_vaddr & ~PAGE_MASK) != offset_in_page) {
				vfs_close(f);
				printk("do_exec_elf: Bad offsets in segment %d '%s'\n", i, path);
				return -ENOEXEC;
			}
			u32 file_offset = phdr.p_offset - offset_in_page;
			u32 virt_addr = phdr.p_vaddr - offset_in_page;
			u32 read_size = phdr.p_filesz + offset_in_page;
			u32 mem_size = phdr.p_memsz + offset_in_page;
			if (virt_addr > KERNEL_VIRT_ADDR) {
				printk("do_exec_elf: Bad virt addr 0x%x in segment %d '%s' (%d)\n", virt_addr, i, path);
				return -ENOEXEC;
			}
			u32 prot = elf_flags_to_mmap_prot(phdr.p_flags);
			ret = PTR_ERR(do_mmap(f, file_offset, virt_addr, read_size, prot, MAP_PRIVATE|MAP_FIXED));
			if (IS_ERR(ERR_PTR(ret))) {
				printk("do_exec_elf: do_mmap(file) failed in segment %d '%s' (%d)\n", i, path, ret);
				return ret;
			}
			if (mem_size > ROUND_PAGE_UP(read_size)) {
				ret = PTR_ERR(do_mmap(NULL, 0, virt_addr + ROUND_PAGE_UP(read_size), mem_size-ROUND_PAGE_UP(read_size), prot, MAP_PRIVATE|MAP_FIXED));
				if (IS_ERR(ERR_PTR(ret)) < 0) {
					printk("do_exec_elf: do_mmap(mem) failed in segment %d '%s' (%d)\n", i, path, ret);
					return ret;
				}
			}
			break;
		case PT_GNU_STACK:
			stack_flags = phdr.p_flags;
			break;
		default:
			vfs_close(f);
			printk("do_exec_elf: Unknown segment type %d in segment %d '%s'\n", phdr.p_type, i, path);
			return -ENOEXEC;
		}

	}
	vfs_close(f);

	/* setup stack */
	ret = PTR_ERR(do_mmap(NULL, 0, STACK_START - STACK_SIZE, STACK_SIZE, elf_flags_to_mmap_prot(stack_flags), MAP_PRIVATE|MAP_FIXED));
	if (IS_ERR(ERR_PTR(ret))) {
		printk("do_exec_elf: do_mmap failed for stack '%s' (%d)\n", path, ret);
		return ret;
	}

	regs_t *r = (regs_t *)((task_stack_t *)current + 1) - 1;
	r->eax = r->ebx = r->ecx = r->edx = r->esi = r->edi = r->ebp = 0;
	r->cs = USER_CS;
	r->ds = r->es = r->fs = r->gs = r->ss = USER_DS;
	r->eip = ehdr.e_entry;
	r->esp = STACK_START;
	r->eflags = (1<<9) | (1<<1);	// 9 is IF, 1 is always on
	printk("do_exec: success\n");
	return 0;
}

int sys_exec(const char *path)
{
	return do_exec_elf(path);
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
