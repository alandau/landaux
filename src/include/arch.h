#ifndef ARCH_H
#define ARCH_H

#define KERNEL_CS	0x0008
#define KERNEL_DS	0x0010
#define LDT_SELECTOR	0x0018
#define TSS_SELECTOR	0x0020

#define USER_CS		0x000F
#define USER_DS		0x0017

#ifndef ASM

#include <stddef.h>

typedef struct regs_struct
{
	u32 edx, ecx, ebx, esi, edi, ebp;
	u16 ds, __ds, es, __es, fs, __fs, gs, __gs;
	u32 eax, intnum, error_code;
	u32 eip;
	u16 cs, __cs;
	u32 eflags;
	u32 esp;
	u16 ss, __ss;
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

static inline void cli(void)
{
	__asm__ __volatile__ ("cli");
}

static inline void sti(void)
{
	__asm__ __volatile__ ("sti");
}

static inline u32 save_flags(void)
{
	u32 flags;
	__asm__ __volatile__ ("pushf\n\tpopl %0" : "=g" (flags) : /* no input */);
	return flags;
}

static inline u32 save_flags_irq(void)
{
	u32 flags = save_flags();
	cli();
	return flags;
}

static inline void restore_flags(u32 flags)
{
	__asm__ __volatile__ ("pushl %0\n\tpopf" : /* no output */ : "g" (flags) : "memory", "cc");
}

static inline void halt(void)
{
	 __asm__ __volatile__ ("hlt");
}

static inline u32 get_esp(void)
{
	u32 ret;
	__asm__ __volatile__ ("mov %%esp, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline u32 get_ebp(void)
{
	u32 ret;
	__asm__ __volatile__ ("mov %%ebp, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline u32 get_cr0(void)
{
	u32 ret;
	__asm__ __volatile__ ("mov %%cr0, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline u32 get_cr2(void)
{
	u32 ret;
	__asm__ __volatile__ ("mov %%cr2, %0" : "=r" (ret) : /* no input */);
	return ret;
}

static inline void set_cr0(u32 cr0)
{
	__asm__ __volatile__ ("mov %0, %%cr0" : /* no output */ : "r" (cr0));
}

static inline void set_cr3(u32 cr3)
{
	__asm__ __volatile__ ("mov %0, %%cr3" : /* no output */ : "r" (cr3));
	__asm__ __volatile__ ("jmp 1f\n\t1:");
}

static inline void move_to_user_mode(void)
{
	__asm__ __volatile__ (
		"movl %1, %%eax\n\t"	// USER_DS
		"movw %%ax, %%ds\n\t"
		"movw %%ax, %%es\n\t"
		"movw %%ax, %%fs\n\t"
		"movw %%ax, %%gs\n\t"
		"pushl %1\n\t"			// USER_DS
		"pushl %%esp\n\t"
		"pushfl\n\t"
		"pushl %0\n\t"			// USER_CS
		"pushl $1f\n\t"
		"iret\n\t"
		"1:"
		: /* no output */
		: "i" (USER_CS), "i" (USER_DS)
		: "eax"
	);
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
