#include <video.h>
#include <arch.h>
#include <syms.h>
#include <mm.h>

#define MAX_CALL_TRACE		100
#define MAX_DATA_TRACE		10

static inline int is_code(unsigned long address)
{
	extern char code[], code_end[];
	return (unsigned long)code <= address && address < (unsigned long)code_end;
}

/* prints call stack in symbolic form */
void print_stack(void)
{
	register int i = MAX_CALL_TRACE;
	register symbol_t *sym;
	register unsigned long *esp = (unsigned long *)get_esp();
//	register unsigned long *orig_esp = esp;
	register unsigned long page = (unsigned long)esp & ~(PAGE_SIZE-1);
	printk("Call Stack:\n");
	while (i-- && page == ((unsigned long)esp & ~(PAGE_SIZE-1)))
	{
		sym = find_symbol(*esp);
		if (is_code(*esp))
			printk("[%0X] %0X %s\n", esp, *esp, sym->symbol);
		esp++;
	}
	/*
	printk("\nStack:\n");
	esp = orig_esp;
	i = MAX_DATA_TRACE;
	while (i--)
	{
		printk("[%0X] %0X\n", esp, *esp);
		esp++;
	}
	*/
}

void dump(const regs_t *r)
{
	printk( "EAX=%0X\tEBX=%0X\tECX=%0X\tEDX=%0X\n"
		"ESI=%0X\tEDI=%0X\tEBP=%0X\tErr=%0X\n"
		"CS= %0X\tDS= %0X\tES= %0X\tFS= %0X\tGS= %0X\n"
		"EIP=%0X\tEFLAGS=%0X\n",
		r->eax, r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp, r->error_code,
		r->cs, r->ds, r->es, r->fs, r->gs, r->eip, r->eflags);
	print_stack();
}

void oops(const regs_t *r)
{
	dump(r);
	cli();
	halt();
}
