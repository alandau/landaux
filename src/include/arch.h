#ifndef ARCH_H
#define ARCH_H

#include <stddef.h>

typedef struct regs_struct
{
	unsigned long ebx, ecx, edx, esi, edi, ebp;
	unsigned long ds, es, fs, gs;
	unsigned long eax, error_code;
	unsigned long eip, cs, eflags;
	/* unsigned long old_esp, old_ss; */
} regs_t;

static inline void outb(unsigned short port, unsigned char value)
{
	__asm__ __volatile__ ("outb %%al, %%dx" : /* no output */ : "a" (value), "d" (port));
}

static inline unsigned char inb(unsigned short port)
{
	unsigned char ret;
	__asm__ __volatile__ ("inb %%dx, %%al" : "=a" (ret) : "d" (port));
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

static inline unsigned long save_flags(void)
{
	unsigned long flags;
	__asm__ __volatile__ ("pushf\n\tpopl %0" : "=g" (flags) : /* no input */);
	return flags;
}

static inline void restore_flags(unsigned long flags)
{
	__asm__ __volatile__ ("pushl %0\n\tpopf" : /* no output */ : "g" (flags) : "memory", "cc");
}

static inline void halt(void)
{
	__asm__ __volatile__ ("hlt");
}

static inline unsigned long get_esp(void)
{
	unsigned long ret;
	__asm__ __volatile__ ("mov %%esp, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline unsigned long get_cr0(void)
{
	unsigned long ret;
	__asm__ __volatile__ ("mov %%cr0, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline void set_cr0(unsigned long cr0)
{
	__asm__ __volatile__ ("mov %0, %%cr0" : /* no output */ : "r" (cr0));
}

static inline void set_cr3(unsigned long cr3)
{
	__asm__ __volatile__ ("mov %0, %%cr3" : /* no output */ : "r" (cr3));
	__asm__ __volatile__ ("jmp 1f\n\t1:");
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
	extern unsigned short masked_irqs;
	if (masked_irqs & (1 << irq)) return;
	masked_irqs |= (1 << irq);
	if (irq < 8)
		outb(PIC1 + 1, masked_irqs & 0xFF);
	else
		outb(PIC2 + 1, masked_irqs >> 8);
}

static inline void unmask_irq(int irq)
{
	extern unsigned short masked_irqs;
	if (!(masked_irqs & (1 << irq))) return;
	masked_irqs &= ~(1 << irq);
	if (irq < 8)
		outb(PIC1 + 1, masked_irqs & 0xFF);
	else
		outb(PIC2 + 1, masked_irqs >> 8);
}

#endif
