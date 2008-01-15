#include <arch.h>

/* Every emtry is 16 bytes long */

#define jmp32(x)	.byte 0xe9; .long ((x) - . - 4);

#define IRQ(x)			\
	pushl $0;		\
	pushl $x;		\
	pushl %eax;		\
	movl $do_irq, %eax;	\
	jmp32(irq_common);	\
	nop;

#define EXC(x, func)		\
	pushl $0;		\
	pushl $x;		\
	pushl %eax;		\
	movl $func, %eax;	\
	jmp32(irq_common);	\
	nop;

/* D = default */
#define EXCD(x)		EXC(x, do_unknown_exception)

#define EXC_E(x, func)		\
	pushl $x;		\
	pushl %eax;		\
	movl $func, %eax;	\
	jmp32(irq_common);	\
	nop; nop; nop;

/* D = default */
#define EXC_ED(x)	EXC_E(x, do_unknown_exception)

.align 4
.global irq_entry_points_start
irq_entry_points_start:
/* Hardware exceptions */
EXC(0, do_divide)		EXCD(1)				EXCD(2)				EXCD(3)
EXCD(4)				EXCD(5)				EXCD(6)				EXCD(7)
EXC_E(8, do_double_fault)	EXCD(0x09)			EXC_E(10, do_tss)		EXC_ED(11)
EXC_E(12, do_stack)		EXC_E(13, do_general_prot)	EXC_E(14, do_page_fault)	EXCD(15)
EXCD(15)			EXC_ED(17)			EXCD(18)			EXCD(19)
EXCD(20)			EXCD(21)			EXCD(22)			EXCD(23)
EXCD(24)			EXCD(25)			EXCD(26)			EXCD(27)
EXCD(28)			EXCD(29)			EXCD(30)			EXCD(31)
/* IRQs */
IRQ(0x20)	IRQ(0x21)	IRQ(0x22)	IRQ(0x23)
IRQ(0x24)	IRQ(0x25)	IRQ(0x26)	IRQ(0x27)
IRQ(0x28)	IRQ(0x29)	IRQ(0x2a)	IRQ(0x2b)
IRQ(0x2c)	IRQ(0x2d)	IRQ(0x2e)	IRQ(0x2f)
/* System call */
IRQ(0x30)
.global irq_entry_points_end
irq_entry_points_end:


irq_common:
pushl %gs
pushl %fs
pushl %es
pushl %ds
pushl %ebp
pushl %edi
pushl %esi
pushl %ebx
pushl %ecx
pushl %edx
movw $KERNEL_DS, %dx
movw %dx, %ds
movw %dx, %es
movw %dx, %fs
movw %dx, %gs
call *%eax
popl %edx
popl %ecx
popl %ebx
popl %esi
popl %edi
popl %ebp
popl %ds
popl %es
popl %fs
popl %gs
popl %eax
addl $8, %esp	/* Pop intnum and error_code */
iret
