#include <kernel.h>

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

void do_coprocessor_err(regs_t r)
{
	printk("Coprocessor error\n");
	oops(&r);
}

void do_unknown_exception(regs_t r)
{
	const char *p = NULL;
	switch (r.intnum) {
		case  0: p = "Divide error"; break;
		case  1: p = "Debug exception"; break;
		case  2: p = "NMI interrupt"; break;
		case  3: p = "Breakpoint"; break;
		case  4: p = "Overflow"; break;
		case  5: p = "Bounds"; break;
		case  6: p = "Invalid opcode"; break;
		case  7: p = "No math coprocessor"; break;
		case  8: p = "Double fault"; break;
		case  9: p = "Coprocessor segment overrun"; break;
		case 10: p = "Invalid TSS"; break;
		case 11: p = "Segment not present"; break;
		case 12: p = "Stack segment fault"; break;
		case 13: p = "General protection fault"; break;
		case 14: p = "Page fault"; break;
		case 15: p = "Reserved"; break;
	}
	if (p)
		printk("Exception #%d: %s\n", r.intnum, p);
	else
		printk("Unknown exception #%d.\n", r.intnum);
	oops(&r);
}
