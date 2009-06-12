#include <kernel.h>
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
	symbol_t *sym;
	if (r) {
		sym = find_symbol(r->eip);
		printk("At 0x%x %s+0x%x/0x%x [0x%x] \n", sym->address, sym->symbol,
			r->eip - sym->address, (sym+1)->address-sym->address, r->eip);
	}
	int i = MAX_CALL_TRACE;
	/* Use r->ebp only if registers are set and it's kernel mode */
	u32 *ebp = (u32 *)(r && (r->cs & 3) == 0 ? r->ebp : get_ebp());
	u32 page = (u32)ebp & ~(PAGE_SIZE-1);
	printk("Call Stack:\n");
	while (i-- && page == ((u32)ebp & ~(PAGE_SIZE-1))) {
		u32 addr = *(ebp+1);
		if (is_code(addr)) {
			sym = find_symbol(addr);
			printk("0x%x %s+0x%x/0x%x [0x%x]\n", sym->address, sym->symbol,
				addr - sym->address, (sym+1)->address-sym->address, addr);
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
	printk("EAX=%08x\tEBX=%08x\tECX=%08x\tEDX=%08x\n"
		"ESI=%08x\tEDI=%08x\tEBP=%08x\tErr=%08x\n"
		"CS= %08x\tDS= %08x\tES= %08x\tFS= %08x\tGS= %08x\n"
		"EIP=%08x\tEFLAGS=%08x\tSS =%08x\tESP=%08x\n",
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
