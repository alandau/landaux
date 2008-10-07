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

#define GDT_MAX_ENTRIES 3
#define IDT_MAX_ENTRIES (32+16+1)

void f(void **a, int count)
{
	int i;
	for (i=0; i<1000*count; i++)
	{
		a[i] = alloc_page(MAP_READWRITE);
		if (!a[i]) {printk("i=%d\n", i); BUG();}
	}
}

void g(void **a, int count)
{
	int i;
	for (i=0; i<1000*count; i++)
	{
		free_page(a[i]);
	}
}

void test_mm(void)
{
	void **a;
	u32 f1, f2, f3;
	f1 = get_free_mem();
	a = alloc_pages(3, MAP_READWRITE);
//	f(a, 3);
	f2 = get_free_mem();
//	g(a, 3);
	free_pages(a, 3);
	f3 = get_free_mem();
	printk("f1=%u\nf2=%u\nf3=%u\n", f1, f2, f3);
}

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

static void kthread_func(void *data)
{
	while (1) {
		int x=0;
		while (x++ < 100)
			printk("%d", current->pid);
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

static void init_gdt(void)
{
	/* entry 0 is null */
	enum {TYPE_CODE = 0xA, TYPE_DATA = 0x2};
	setup_gdt_entry(KERNEL_CS, TYPE_CODE, 1, 0);
	setup_gdt_entry(KERNEL_DS, TYPE_DATA, 1, 0);
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

void doit(char *path)
{
	dentry_t *d = lookup_path(path);
	printk("%s: %s\n", path, d ? "yes" : "no");
	if (d)
		dentry_put(d);
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
	/* mbi->mem_upper is the amount of memory (in KB) above 1MB */
	printk("Found %lu MB of memory.\n", mbi->mem_upper/1024 + 1);
	init_gdt();
	init_idt();
	base = ROUND_PAGE_UP(V2P((unsigned long)_end));
	size = ROUND_PAGE_DOWN(mbi->mem_upper*1024 - base);
	init_pmm(base, size);
	map_memory((mbi->mem_upper+1024) * 1024);	/* map all memory */
	init_heap();
	init_pic();
	init_pit();
//	init_mm();
//	init_idle();
//	init_tss();
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
	doit("/");
	doit("/a");
	doit("/a/b");
	doit("/a/b/c");
	doit("/x");
	doit("/b");
	doit("/c");
	doit("/c/1");
	doit("/c/2");

	return;
	
	kernel_thread(kthread_func, "1");
	kernel_thread(kthread_func, "2");
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
