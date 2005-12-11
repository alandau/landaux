#include <kernel.h>
#include <video.h>

void do_divide(regs_t r)
{
	printk("Divide by zero\n");
	oops(&r);
}

void do_debug(regs_t r)
{
	printk("Debug exception\n");
	oops(&r);
}

void do_breakpoint(regs_t r)
{
	printk("Breakpoint\n");
	oops(&r);
}

void do_overflow(regs_t r)
{
	printk("Overflow\n");
	oops(&r);
}

void do_bounds(regs_t r)
{
	printk("Bounds exception\n");
	oops(&r);
}

void do_opcode(regs_t r)
{
	printk("Invalid opcode\n");
	oops(&r);
}

void do_coprocessor_na(regs_t r)
{
	printk("Coprocessor not available\n");
	oops(&r);
}

void do_double_fault(regs_t r)
{
	printk("Double fault\n");
	oops(&r);
}

void do_coprocessor_seg(regs_t r)
{
	printk("Coprocessor segment error\n");
	oops(&r);
}

void do_tss(regs_t r)
{
	printk("Invalid TSS\n");
	oops(&r);
}

void do_segment_na(regs_t r)
{
	printk("Segment not present\n");
	oops(&r);
}

void do_stack(regs_t r)
{
	printk("Stack overflow\n");
	oops(&r);
}

void do_general_prot(regs_t r)
{
	printk("General protection fault\n");
	oops(&r);
}

void do_page_fault(regs_t r)
{
	printk("Page fault\n");
	printk("Trying to %s address: 0x%0x\n", (r.error_code & 0x02?"write to":"read from"), get_cr2());
	oops(&r);
}

void do_coprocessor_err(regs_t r)
{
	printk("Coprocessor error\n");
	oops(&r);
}

void do_reserved(regs_t r)
{
	printk("Reserved exception\n");
	oops(&r);
}
