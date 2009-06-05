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

#define GDT_MAX_ENTRIES 6
#define IDT_MAX_ENTRIES (32+16+1)

int sys_ni_syscall(void)
{
	return -1;
}

static inline int user_fork(void)
{
	int ret;
	__asm__ __volatile__ ("int $0x30" : "=a"(ret) : "0"(1));
	return ret;
}

static void run_init(void *data)
{
	printk("Running /prog...\n");
	kernel_exec("/prog");
}

static void kthread_func(void *data)
{
	while (1) {
#if 0
		int x=0;
		while (x++ < 100)
#endif
			printk("%d", current->pid);
		if (current->timeslice == 0)
			schedule();
	}
}

struct gdtr {
	unsigned short	limit;
	unsigned long	addr;
} __attribute__((packed));
typedef char BUG_struct_gdtr_size_not_6[sizeof(struct gdtr) == 6 ? 1 : -1];

struct gdt_entry {
	unsigned short	limit0_15;
	unsigned short	base0_15;
	unsigned char	base16_23;
	unsigned char	type : 4;
	unsigned char	regular : 1;
	unsigned char	dpl : 2;
	unsigned char	present : 1;
	unsigned char	limit16_19 : 4;
	unsigned char	avail : 1;
	unsigned char	reserved : 1;
	unsigned char	opsize : 1;
	unsigned char	granularity : 1;
	unsigned char	base24_31;
} __attribute__((packed));
typedef char BUG_struct_gdt_entry_size_not_8[sizeof(struct gdt_entry) == 8 ? 1 : -1];

static struct gdt_entry gdt[GDT_MAX_ENTRIES] __attribute__((aligned(4)));
static struct gdtr gdtr __attribute__((aligned(4)));

static void setup_gdt_entry(unsigned int selector, int type, int regular, int dpl)
{
	unsigned int entry = selector / 8;
	BUG_ON(entry >= GDT_MAX_ENTRIES);
	struct gdt_entry *e = &gdt[entry];
	memset(e, 0, sizeof(struct gdt_entry));
	e->limit0_15 = 0xffff;
	e->type = type;
	e->regular = regular;
	e->dpl = dpl;
	e->present = 1;
	e->limit16_19 = 0xf;
	e->opsize = 1;
	e->granularity = 1;
#if 0
	/* zeroed by memset above */
	e->base0_15 = 0;
	e->base16_23 = 0;
	e->avail = 0;
	e->reserved = 0;
	e->base24_31 = 0;
#endif
}

static void setup_tss_gdt_entry(unsigned int selector, u32 base, u32 size)
{
	enum {TYPE_TSS = 0x9};
	unsigned int entry = selector / 8;
	BUG_ON(entry >= GDT_MAX_ENTRIES);
	struct gdt_entry *e = &gdt[entry];
	memset(e, 0, sizeof(struct gdt_entry));
	e->type = TYPE_TSS;
	e->present = 1;
	e->granularity = 0;
	e->base0_15 = base & 0xffff;
	e->base16_23 = (base >> 16) & 0xff;
	e->base24_31 = base >> 24;
	size--;		/* limit == size-1 */
	e->limit0_15 = size & 0xffff;
	e->limit16_19 = (size >> 16) & 0x0f;
#if 0
	/* zeroed by memset above */
	e->avail = 0;
	e->reserved = 0;
	e->regular = 0;
	e->dpl = 0;
	e->opsize = 0;
#endif
}

static void init_gdt(void)
{
	/* entry 0 is null */
	enum {TYPE_CODE = 0xA, TYPE_DATA = 0x2};
	extern tss_t tss;
	setup_gdt_entry(KERNEL_CS, TYPE_CODE, 1, 0);
	setup_gdt_entry(KERNEL_DS, TYPE_DATA, 1, 0);
	setup_gdt_entry(USER_CS, TYPE_CODE, 1, 3);
	setup_gdt_entry(USER_DS, TYPE_DATA, 1, 3);
	setup_tss_gdt_entry(TSS_SELECTOR, (u32)&tss, sizeof(tss));
	gdtr.limit = sizeof(gdt) - 1;
	gdtr.addr = (unsigned long)&gdt;
	__asm__ __volatile__ (
		"lgdt %0\n\t"
		"movw %1, %%ax\n\t"
		"movw %%ax, %%ds\n\t"
		"movw %%ax, %%es\n\t"
		"movw %%ax, %%fs\n\t"
		"movw %%ax, %%gs\n\t"
		"movw %%ax, %%ss\n\t"
		"ljmp %2, $1f\n\t"
		"1:\n\t"
		: /* no output */ : "m"(gdtr), "i"(KERNEL_DS), "i"(KERNEL_CS) : "eax"
	);
}

struct idtr {
	unsigned short	limit;
	unsigned long	addr;
} __attribute__((packed));
typedef char BUG_struct_idtr_size_not_6[sizeof(struct idtr) == 6 ? 1 : -1];

struct idt_entry {
	unsigned short	offset0_15;
	unsigned short	selector;
	unsigned char	reserved;
	unsigned char	trapgate : 1;
	unsigned char	ones : 3;
	unsigned char	zero : 1;
	unsigned char	dpl : 2;
	unsigned char	present : 1;
	unsigned short	offset16_31;
} __attribute__((packed));
typedef char BUG_struct_idt_entry_size_not_8[sizeof(struct idt_entry) == 8 ? 1 : -1];

static struct idt_entry idt[IDT_MAX_ENTRIES] __attribute__((aligned(4)));
static struct idtr idtr __attribute__((aligned(4)));

void setup_idt_entry(struct idt_entry *e, void *addr, int trapgate, int dpl)
{
	memset(e, 0, sizeof(struct idt_entry));
	e->offset0_15 = (unsigned long)addr & 0xffff;
	e->offset16_31 = (unsigned long)addr >> 16;
	e->selector = KERNEL_CS;
	e->trapgate = trapgate;
	e->ones = ~(unsigned char)0;
	e->dpl = dpl;
	e->present = 1;
}

static void init_idt(void)
{
	extern char irq_entry_points_start[], irq_entry_points_end[];
	int i;
	/* each "routine" takes 16 bytes */
	BUG_ON(irq_entry_points_end-irq_entry_points_start != 16*IDT_MAX_ENTRIES);

	/* Exceptions */
	for (i = 0; i < 32; i++)
		setup_idt_entry(&idt[i], irq_entry_points_start + i*16, 1, 0);
	/* IRQs */
	for (i = 0x20; i < 0x30; i++)
		setup_idt_entry(&idt[i], irq_entry_points_start + i*16, 0, 0);
	/* Syscall */
	setup_idt_entry(&idt[0x30], irq_entry_points_start + 0x30*16, 0, 3);

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
		printk("Cmdline = <%s> %p\n", P2V(mbi->cmdline), P2V(mbi->cmdline));
	}
	if (mbi->flags & 8) {
		printk("Bootmods = %d\n", mbi->mods_count);
		module_t* m = (module_t *)P2V(mbi->mods_addr);
		for (i = 0; i < mbi->mods_count; i++, m++)
			printk(" <%s> %p %p-%p\n", P2V(m->string), P2V(m->string), m->mod_start, m->mod_end);
	}
	if (mbi->flags & 64) {
		unsigned long next = mbi->mmap_addr;
		unsigned long size = mbi->mmap_length;
		printk("Memory map:\n");
		while (next < mbi->mmap_addr + size) {
			memory_map_t *mm = (memory_map_t *)P2V(next);
			next += mm->size + 4;
			printk(" start=%p, len=%d, type=%d\n", mm->base_addr_low, mm->length_low, mm->type);
		}

	}
	if (mbi->flags & 512) {
		unsigned long name = *(unsigned long *)((unsigned long)mbi + 64);
		printk("Bootldr name = <%s> %p\n", P2V(name), P2V(name));
	}
}
#endif

static unsigned long get_modules_end(multiboot_info_t *mbi)
{
	if (!(mbi->flags & 8))
		return 0;
	if (mbi->mods_count == 0)
		return 0;
	module_t* m = (module_t *)P2V(mbi->mods_addr);
	return ROUND_PAGE_UP(m[mbi->mods_count-1].mod_end);
}

static void extract_bootimg(multiboot_info_t *mbi)
{
	int extract_tar(u32 start, u32 size);
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
	while (1);
#endif
	/* mbi->mem_upper is the amount of memory (in KB) above 1MB */
	printk("Found %lu MB of memory.\n", mbi->mem_upper/1024 + 1);
	init_gdt();
	init_idt();
	base = get_modules_end(mbi);
	if (!base)
		base = ROUND_PAGE_UP(V2P((unsigned long)_end));
	size = ROUND_PAGE_DOWN(mbi->mem_upper*1024 - base);
	init_pmm(base, size);
	init_idle();
	map_memory((mbi->mem_upper+1024) * 1024);	/* map all memory */
	init_heap();
	init_pic();
	init_pit();
	init_mm((mbi->mem_upper+1024) * 1024);
	init_tss();
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
	tree("/");
#if 0
	kernel_thread(kthread_func, "1");
	kernel_thread(kthread_func, "2");
#endif
	kernel_thread(run_init, NULL);
	kernel_thread(kthread_func, NULL);
	schedule();
	
#if 0
	move_to_user_mode();
	if (user_fork() == 0)
	{
		// child
		char *str="in init\n";
		int tmp;
		//printk(str);
		__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(0), "b"(str));
		__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(2), "b"(str));
		while (1);
	}

	// parent is the idle task
	{
		const char *str="in idle\n";
		int tmp;
		__asm__ __volatile__ ("int $0x30" : "=a"(tmp) : "a"(0), "b"(str));
	}
#endif	
	/* idle loop */
	while (1) halt();
}
