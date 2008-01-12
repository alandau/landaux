#include <video.h>
#include <arch.h>
#include <syms.h>
#include <mm.h>
#include <process.h>

#define MAX_CALL_TRACE		100
#define MAX_DATA_TRACE		10

static inline int is_code(u32 address)
{
	extern char _code[], _code_end[];
	return (u32)_code <= address && address < (u32)_code_end;
}

/* prints call stack in symbolic form */
void print_stack(const regs_t *r)
{
	int i = MAX_CALL_TRACE;
	u32 *ebp = (u32 *)(r ? r->ebp : get_ebp());
	u32 page = (u32)ebp & ~(PAGE_SIZE-1);
	symbol_t *sym;
	if (r)
	{
		sym = find_symbol(r->eip);
		printk("At %0X %s+0x%x/0x%x [%0X] \n", sym->address, sym->symbol, r->eip - sym->address, (sym+1)->address-sym->address, r->eip);
	}
	printk("Call Stack:\n");
	while (i-- && page == ((u32)ebp & ~(PAGE_SIZE-1)))
	{
		u32 addr = *(ebp+1);
		if (is_code(addr))
		{
			sym = find_symbol(addr);
			printk("%0X %s+0x%x/0x%x [%0X]\n", sym->address, sym->symbol, addr - sym->address, (sym+1)->address-sym->address, addr);
		}
		ebp = (u32 *)*ebp;
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

void dump_regs(const regs_t *r)
{
	printk("EAX=%0X\tEBX=%0X\tECX=%0X\tEDX=%0X\n"
		"ESI=%0X\tEDI=%0X\tEBP=%0X\tErr=%0X\n"
		"CS= %0X\tDS= %0X\tES= %0X\tFS= %0X\tGS= %0X\n"
		"EIP=%0X\tEFLAGS=%0X\tSS =%0X\tESP=%0X\n",
		r->eax, r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp, r->error_code,
		r->cs, r->ds, r->es, r->fs, r->gs, r->eip, r->eflags, r->ss, r->esp);
}

void oops(const regs_t *r)
{
	printk("PID %d\n", current->pid);
	dump_regs(r);
	print_stack(r);
	cli();
	halt();
}
