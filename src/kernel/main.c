#include <multiboot.h>
#include <arch.h>
#include <console.h>
#include <mm.h>
#include <syms.h>
#include <kernel.h>
#include <irq.h>
#include <process.h>
#include <sched.h>
#include <block.h>
#include <string.h>
#include <fs.h>

static void run_init(void *data)
{
	printk("Running /prog...\n");
	kernel_exec("/prog");
}

static void kthread_func(void *data)
{
	printk("int kthreda_func\n");
	int x=0;
	while (1) {
		if (need_resched()) {
			if (x++ % 10 == 0)
				printk("%d%s\n", current->pid, data);
			schedule();
		}
	}
}

struct idtr {
	unsigned short	limit;
	unsigned long	addr;
} __attribute__((packed));
typedef char BUG_struct_idtr_size_not_10[sizeof(struct idtr) == 10 ? 1 : -1];

struct idt_entry {
	unsigned short	offset0_15;
	unsigned short	selector;
	unsigned char	ist : 3;
	unsigned char	reserved : 5;
	unsigned char   trapgate : 1;
	unsigned char	ones : 3;
	unsigned char	zero : 1;
	unsigned char	dpl : 2;
	unsigned char	present : 1;
	unsigned short	offset16_31;
	unsigned int	offset32_63;
	unsigned int	ignore;
} __attribute__((packed));
typedef char BUG_struct_idt_entry_size_not_16[sizeof(struct idt_entry) == 16 ? 1 : -1];

static struct idt_entry idt[256] __attribute__((aligned(8)));
static struct idtr idtr __attribute__((aligned(4)));

void setup_idt_entry(struct idt_entry *e, void *addr, int trapgate)
{
	memset(e, 0, sizeof(struct idt_entry));
	e->offset0_15 = (unsigned long)addr & 0xffff;
	e->offset16_31 = ((unsigned long)addr >> 16) & 0xffff;
	e->offset32_63 = (unsigned long)addr >> 32;
	e->selector = KERNEL_CS;
	e->trapgate = trapgate;
	e->ones = ~(unsigned char)0;
	e->present = 1;
}

static void init_idt(void)
{
	extern char vector_entry_points_start[], vector_entry_points_end[];
	int i;
	/* each "routine" takes 16 bytes */
	BUG_ON(vector_entry_points_end - vector_entry_points_start != 16 * 256);

	/* Exceptions */
	for (i = 0; i < 32; i++)
		setup_idt_entry(&idt[i], vector_entry_points_start + i*16, 1);
	idt[14].trapgate = 0;	/* page fault */
	/* IRQs */
	for (i = 32; i < 256; i++)
		setup_idt_entry(&idt[i], vector_entry_points_start + i*16, 0);

	idtr.limit = sizeof(idt) - 1;
	idtr.addr = (unsigned long)&idt;
	__asm__ __volatile__ ("lidt %0\n\t" : /* no output */ : "m"(idtr));
}

void tree(char *path)
{
	char buf[50];
	int i;
	user_dentry_t *dp;
	dentry_t *d = lookup_path_dir(path);
	if (IS_ERR(d))
		return;
	printk("D %7d %s\n", 0, path);
	int count, start = 0;
	while (1) {
		count = vfs_dgetdents(d, buf, 50, start);
		if (count < 0) {
			printk("count=%d\n", count);
			break;
		}
		if (count == 0)
			break;
		dp = (user_dentry_t *)buf;
		for (i = 0; i < count; i++) {
			if (DIR_TYPE(dp->mode) == DIR_DIR) {
				char *s = kmalloc(strlen(path) + 1 + strlen(dp->name) + 1);
				s[0] = '\0';
				strcpy(s, path);
				if (path[strlen(path)-1] != '/')
					strcat(s, "/");
				strcat(s, dp->name);
				tree(s);
				kfree(s);
			} else {
				printk("F %7d %s%s%s\n", dp->size, path, path[strlen(path)-1]=='/'?"":"/", dp->name);
			}
			dp = (user_dentry_t *)(buf + dp->reclen);
		}
		start += count;
	}
	dentry_put(d);
}

#if 0
static void dump_mbi(multiboot_info_t *mbi)
{
	const char *flags[] = {
		"memsize", "bootdev", "cmdline", "bootmods",
		"syms-aout", "syms-elf", "memmap", "drives",
		"config", "bootldr", "apm", "vbe"
	};
	int i;
	printk("Flags = 0x%x\n    ", mbi->flags);
	for (i = 0; i < sizeof(flags)/sizeof(flags[0]); i++)
		if (mbi->flags & (1<<i))
			printk("%s ", flags[i]);
	printk("\n");
	if (mbi->flags & 1) {
		printk("Mem lower = %d KB\nMem upper = %d KB\n", mbi->mem_lower, mbi->mem_upper);
	}
	if (mbi->flags & 4) {
		printk("Cmdline = <%s> %p\n", P2V_IMAGE(mbi->cmdline), P2V_IMAGE(mbi->cmdline));
	}
	if (mbi->flags & 8) {
		printk("Bootmods = %d\n", mbi->mods_count);
		module_t* m = (module_t *)P2V_IMAGE(mbi->mods_addr);
		for (i = 0; i < mbi->mods_count; i++, m++)
			printk(" <%s> %p %p-%p\n", P2V_IMAGE(m->string), P2V_IMAGE(m->string), m->mod_start, m->mod_end);
	}
	if (mbi->flags & 64) {
		unsigned int next = mbi->mmap_addr;
		unsigned int size = mbi->mmap_length;
		printk("Memory map:\n");
		while (next < mbi->mmap_addr + size) {
			memory_map_t *mm = (memory_map_t *)P2V_IMAGE(next);
			next += mm->size + 4;
			printk(" start=%x, len=%d, type=%d\n", mm->base_addr_low, mm->length_low, mm->type);
		}

	}
	if (mbi->flags & 512) {
		unsigned long name = *(unsigned long *)((unsigned long)mbi + 64);
		printk("Bootldr name = <%s> %p\n", P2V_IMAGE(name), P2V_IMAGE(name));
	}
}
#endif

static unsigned long get_modules_end(multiboot_info_t *mbi)
{
	if (!(mbi->flags & 8))
		return 0;
	if (mbi->mods_count == 0)
		return 0;
	module_t* m = (module_t *)P2V_IMAGE(mbi->mods_addr);
	return ROUND_PAGE_UP(m[mbi->mods_count-1].mod_end);
}

static void extract_bootimg(multiboot_info_t *mbi)
{
	int extract_tar(void *start, u32 size);
	if (!(mbi->flags & 8)) {
		printk("No bootimg found.\n");
		return;
	}
	if (mbi->mods_count == 0) {
		printk("No bootimg found.\n");
		return;
	}
	module_t* m = (module_t *)P2V(mbi->mods_addr);
	int i;
	for (i = 0; i < mbi->mods_count; i++, m++) {
		int ret = extract_tar(P2V(m->mod_start), m->mod_end - m->mod_start);
		if (ret < 0)
			printk("Error extracting bootimg '%s': %d\n", P2V(m->string), ret);
	}
}

void init_syscall(void) {
	void system_call(void);
	wrmsr(0xc0000081, ((u64)(USER_CS - 16) << 48) | ((u64)KERNEL_CS << 32));	// STAR
	wrmsr(0xc0000082, (u64)system_call);	/* LSTAR */
	/* Disable TF, DF, IF, IOPL, AC, NT */
	wrmsr(0xc0000084, 0x47700);	/* SFMASK */
}

void run_init_funcs(void) {
	extern const char _init_funcs[], _init_funcs_end[];
	for (u64 *p = (u64*)_init_funcs; p < (u64*)_init_funcs_end; p++) {
		int (*func)(void) = (void* )*p;
		func();
	}
}

/* This is the C entry point of the kernel. It runs with interrupts disabled */
void kernel_start(unsigned long mb_checksum, multiboot_info_t *mbi)
{
	extern char _end[];
	unsigned long base, size;
	init_console();
	if (mb_checksum != MULTIBOOT_BOOTLOADER_MAGIC) {
		printk("Wrong multiboot checksum 0x%08x.\n", mb_checksum);
		while (1);
	}
	if ((mbi->flags & MULTIBOOT_FLAG_MEM_INFO) == 0) {
		printk("No meminfo passed by bootloader.\n");
		while (1);
	}
#if 0
	dump_mbi(mbi);
	halt();
#endif
	/* mbi->mem_upper is the amount of memory (in KB) above 1MB */
	printk("Found %u KB of memory.\n", mbi->mem_upper+1024);
	map_memory((mbi->mem_upper+1024) * 1024);	/* map all memory */
	init_idt();
	init_tss();
	base = get_modules_end(mbi);
	if (!base)
		base = ROUND_PAGE_UP(V2P_IMAGE((unsigned long)_end));
	size = ROUND_PAGE_DOWN(mbi->mem_upper*1024 - base);
	init_pmm(base, size);
	init_idle();
	init_heap();
	init_mm((mbi->mem_upper+1024) * 1024);
	void init_scheduler(void);
	init_scheduler();
	init_pic();
	init_pit();
	void do_keyboard(void *);
	void do_timer(void *);
	register_irq(0, do_timer, NULL);
	register_irq(1, do_keyboard, NULL);
	sti();
	printk("Found %u MB of memory.\n", get_mem_size()/1024/1024);
	printk("Memory used: %u bytes.\n", get_used_mem());

	int init_ramfs(void);
	init_ramfs();
	vfs_mount("ramfs", "/");
	extract_bootimg(mbi);
	int init_devfs(void);
	init_devfs();
	vfs_mkdir("/dev");
	vfs_mount("devfs", "/dev");
	tree("/");
	int init_pci(void);
	init_pci();
	init_syscall();
	run_init_funcs();
#if 0
	kernel_thread(kthread_func, "1");
//	schedule();
	kernel_thread(kthread_func, "2");
#endif
#if 1
	kernel_thread(run_init, NULL);
//	kernel_thread(kthread_func, "kthread");
	schedule();
	
#endif
	/* idle loop */
	while (1) halt();
}
