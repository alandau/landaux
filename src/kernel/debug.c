#include <kernel.h>
#include <arch.h>
#include <syms.h>
#include <mm.h>
#include <process.h>

#define MAX_CALL_TRACE		100

static inline int is_code(u64 address)
{
	extern char _code[], _code_end[];
	return (u64)_code <= address && address < (u64)_code_end;
}

/* prints call stack in symbolic form */
void print_stack(const regs_t *r)
{
	symbol_t *sym;
	if (r) {
		sym = find_symbol(r->rip);
		printk("At %p %s+0x%x/0x%x [%p] \n", sym->address, sym->symbol,
			r->rip - sym->address, (sym+1)->address - sym->address, r->rip);
	}
	int i = MAX_CALL_TRACE;
	/* Use r->rbp only if registers are set and it's kernel mode */
	u64 *rbp = (u64 *)(r && (r->cs & 3) == 0 ? r->rbp : get_rbp());
	u64 page = (u64)rbp & ~(PAGE_SIZE-1);
	printk("Call Stack:\n");
	while (i-- && page == ((u64)rbp & ~(PAGE_SIZE-1))) {
		u64 addr = *(rbp+1);
		if (is_code(addr)) {
			sym = find_symbol(addr);
			printk("%p %s+0x%x/0x%x [%p]\n", sym->address, sym->symbol,
				addr - sym->address, (sym+1)->address - sym->address, addr);
		}
		rbp = (u64 *)*rbp;
	}
}

void dump_regs(const regs_t *r)
{
	printk("RAX=%lx\tRBX=%lx\tRCX=%lx\tRDX=%lx\n"
		"RSI=%lx\tRDI=%lx\tRBP=%lx\tRSP=%lx\n"
		"R8=%lx\tR9=%lx\tR10=%lx\tR11=%lx\n"
		"R12=%lx\tR13=%lx\tR14=%lx\tR15=%lx\n"
		"RIP=%lx\tRFLAGS=%lx\tCS= %x\tSS= %x\tErr=%lx\n",
		r->rax, r->rbx, r->rcx, r->rdx, r->rsi, r->rdi, r->rbp, r->rsp, 
		r->r8, r->r9, r->r10, r->r11, r->r12, r->r13, r->r14, r->r15,
		r->rip, r->rflags, r->cs, r->ss, r->error_code);
}

void oops(const regs_t *r)
{
	printk("PID %d\n", current->pid);
	dump_regs(r);
	print_stack(r);
	cli();
	halt();
}
