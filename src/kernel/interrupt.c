#include <kernel.h>

static void do_unknown_exception(regs_t *r)
{
	const char *p = NULL;
	switch (r->vector) {
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
		printk("Exception #%d: %s\n", r->vector, p);
	else
		printk("Unknown exception #%d.\n", r->vector);
	oops(r);
}

typedef void (*exc_func)(regs_t *r);

void do_page_fault(regs_t *r);

static exc_func exc_func_table[32] = {
	/*  0 */ do_unknown_exception,
	/*  1 */ do_unknown_exception,
	/*  2 */ do_unknown_exception,
	/*  3 */ do_unknown_exception,
	/*  4 */ do_unknown_exception,
	/*  5 */ do_unknown_exception,
	/*  6 */ do_unknown_exception,
	/*  7 */ do_unknown_exception,
	/*  8 */ do_unknown_exception,
	/*  9 */ do_unknown_exception,
	/* 10 */ do_unknown_exception,
	/* 11 */ do_unknown_exception,
	/* 12 */ do_unknown_exception,
	/* 13 */ do_unknown_exception,
	/* 14 */ do_page_fault,
	/* 15 */ do_unknown_exception,
	/* 16 */ do_unknown_exception,
	/* 17 */ do_unknown_exception,
	/* 18 */ do_unknown_exception,
	/* 19 */ do_unknown_exception,
	/* 20 */ do_unknown_exception,
	/* 21 */ do_unknown_exception,
	/* 22 */ do_unknown_exception,
	/* 23 */ do_unknown_exception,
	/* 24 */ do_unknown_exception,
	/* 25 */ do_unknown_exception,
	/* 26 */ do_unknown_exception,
	/* 27 */ do_unknown_exception,
	/* 28 */ do_unknown_exception,
	/* 29 */ do_unknown_exception,
	/* 30 */ do_unknown_exception,
	/* 31 */ do_unknown_exception,
};

extern void do_irq(regs_t *r);

void do_vector(regs_t* r) {
	r->vector += 128;
	if (r->vector < 32) {
		exc_func_table[r->vector](r);
	} else {
		do_irq(r);
	}
}
