#ifndef ARCH_H
#define ARCH_H

#define KERNEL_CS	0x0008
#define KERNEL_DS	0x0010
#define TSS_SELECTOR	0x0018

#define USER_CS		0x0033
#define USER_DS		0x002b

#ifndef ASM

#include <stddef.h>

typedef struct regs_struct
{
	u64 rax, rcx, rdx, rbx, rbp, rsi, rdi;
	u64 r8, r9, r10, r11, r12, r13, r14, r15;
	u64 vector;
	u64 error_code;
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
} regs_t;

static inline void outb(u16 port, u8 value)
{
	__asm__ __volatile__ ("outb %%al, %%dx" : /* no output */ : "a" (value), "d" (port));
}

static inline u8 inb(u16 port)
{
	unsigned char ret;
	__asm__ __volatile__ ("inb %%dx, %%al" : "=a" (ret) : "d" (port));
	return ret;
}

static inline void outl(u16 port, u32 value)
{
	__asm__ __volatile__ ("outl %%eax, %%dx" : /* no output */ : "a" (value), "d" (port));
}

static inline u32 inl(u16 port)
{
	unsigned long ret;
	__asm__ __volatile__ ("inl %%dx, %%eax" : "=a" (ret) : "d" (port));
	return ret;
}

static inline void cli(void)
{
	__asm__ __volatile__ ("cli");
}

static inline void sti(void)
{
	__asm__ __volatile__ ("sti");
}

static inline u64 save_flags(void)
{
	u64 flags;
	__asm__ __volatile__ ("pushf\n\tpop %0" : "=g" (flags) : /* no input */);
	return flags;
}

static inline u64 save_flags_irq(void)
{
	u64 flags = save_flags();
	cli();
	return flags;
}

static inline void restore_flags(u64 flags)
{
	__asm__ __volatile__ ("push %0\n\tpopf" : /* no output */ : "g" (flags) : "memory", "cc");
}

static inline void halt(void)
{
	 __asm__ __volatile__ ("hlt");
}

static inline u64 get_rsp(void)
{
	u64 ret;
	__asm__ __volatile__ ("mov %%rsp, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline u64 get_rbp(void)
{
	u64 ret;
	__asm__ __volatile__ ("mov %%rbp, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline u64 get_cr0(void)
{
	u64 ret;
	__asm__ __volatile__ ("mov %%cr0, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline u64 get_cr2(void)
{
	u64 ret;
	__asm__ __volatile__ ("mov %%cr2, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline void set_cr0(u64 cr0)
{
	__asm__ __volatile__ ("mov %0, %%cr0" : /* no output */ : "r" (cr0));
}

static inline void set_cr3(u64 cr3)
{
	__asm__ __volatile__ ("mov %0, %%cr3" : /* no output */ : "r" (cr3));
	__asm__ __volatile__ ("jmp 1f\n\t1:");
}

static inline u64 rdmsr(u32 msr)
{
	u32 lo, hi;
	__asm__ __volatile__ ("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr));
	return ((u64)hi << 32) | lo;
}

static inline void wrmsr(u32 msr, u64 val)
{
	__asm__ __volatile__ ("wrmsr" : /* no output */ : "c" (msr), "a" (val & 0xffffffff), "d" (val >> 32));
}

static inline u64 rdtsc(void)
{
	u32 lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((u64)hi << 32) | lo;
}

#define PIC1		0x20
#define PIC2		0xA0

#define ICW1		0x11
#define ICW4		0x01

#define PIC1_START	0x20
#define PIC2_START	0x28

#define PIC_ACK		0x20

#define HZ		100

void init_pic(void);
void init_pit(void);

static inline void ack_master_pic(void)
{
	outb(PIC1, PIC_ACK);
}

static inline void ack_slave_pic(void)
{
	outb(PIC1, PIC_ACK);
	outb(PIC2, PIC_ACK);
}

static inline void mask_irq(int irq)
{
	extern u16 masked_irqs;
	if (masked_irqs & (1 << irq)) return;
	masked_irqs |= (1 << irq);
	if (irq < 8)
		outb(PIC1 + 1, masked_irqs & 0xFF);
	else
		outb(PIC2 + 1, masked_irqs >> 8);
}

static inline void unmask_irq(int irq)
{
	extern u16 masked_irqs;
	if (!(masked_irqs & (1 << irq))) return;
	masked_irqs &= ~(1 << irq);
	if (irq < 8)
		outb(PIC1 + 1, masked_irqs & 0xFF);
	else
		outb(PIC2 + 1, masked_irqs >> 8);
}

#endif	/* !ASM */
#endif
