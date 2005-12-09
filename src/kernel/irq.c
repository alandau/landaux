#include <stddef.h>
#include <arch.h>
#include <video.h>
#include <irq.h>
#include <kernel.h>

unsigned long jiffies;

typedef struct
{
	irqhandler_t handler;
	void *data;
	unsigned long count;
} irq_t;

static irq_t irqs[16];

void do_irq(regs_t r)
{
	if (irqs[r.error_code].handler)
	{
		irq_t *irq = &irqs[r.error_code];
		(irq->handler)(irq->data);
		irq->count++;
		if (r.error_code < 8) ack_master_pic();
		else ack_slave_pic();
	}
	else
	{
		printk("Unregistered IRQ #%d\n", r.error_code);
		BUG();
	}
}

int register_irq(int irq, irqhandler_t handler, void *data)
{
	unsigned long flags;
	if (irqs[irq].handler) return -1;
	flags = save_flags();
	irqs[irq].data = data;
	irqs[irq].count = 0;
	irqs[irq].handler = handler;
	unmask_irq(irq);
	restore_flags(flags);
	return 0;
}

int release_irq(int irq)
{
	unsigned long flags;
	if (irqs[irq].handler == NULL) return -1;
	flags = save_flags();
	irqs[irq].handler = NULL;
	mask_irq(irq);
	restore_flags(flags);
	return 0;
}

void do_timer(void *data)
{
	jiffies++;
}

void do_keyboard(void *data)
{
	unsigned char key = inb(0x60);
	printk("keyboard: %s - %d\n", key&0x80?"RELEASED":"PRESSED ",key&0x7F);
}
