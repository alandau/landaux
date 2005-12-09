#include <arch.h>

#define PIT_CONTROL		0x43
#define PIT_CHANNEL0		0x40
#define PIT_CONTROL_VALUE	0x34

#define PIT_FREQ		0x1234DD

unsigned short masked_irqs;

/* initialize and remap PICs */
void init_pic(void)
{
	/* IRQs 0..7 are mapped to interrupts PIC1_START..PIC1_START+7,
	   IRQs 8..15 to PIC2_START..PIC2_START+7 */

	/* initialize the PICs */
	outb(PIC1, ICW1); outb(PIC2, ICW1);
	/* remap the PICs */
	outb(PIC1 + 1, PIC1_START); outb(PIC2 + 1, PIC2_START);
	/* master pic talks to slave via IRQ2 (4=bit 2 on).
	   slave is connected to IRQ2 on master */
	outb(PIC1 + 1, 4); outb(PIC2 + 1, 2);
	/* EOI must be sent with software */
	outb(PIC1 + 1, ICW4); outb(PIC2 + 1, ICW4);

	masked_irqs = 0xFFFB;
	outb(PIC1 + 1, 0xFF & ~(1 << 2));	 /* mask all IRQs except cascading  (IRQ2) */
	outb(PIC2 + 1, 0xFF);
//	outb(PIC1 + 1, 0x00);	/* unmask all IRQs */
}

/* remap PIT to make interrupts at HZ frequency */
void init_pit(void)
{
	unsigned short counter = PIT_FREQ / HZ;
	outb(PIT_CONTROL, PIT_CONTROL_VALUE);
	outb(PIT_CHANNEL0, counter & 0xFF);
	outb(PIT_CHANNEL0, counter >> 8);
}
