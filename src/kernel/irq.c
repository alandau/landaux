#include <stddef.h>
#include <arch.h>
#include <irq.h>
#include <kernel.h>

volatile u64 jiffies;

typedef struct
{
	irqhandler_t handler;
	void *data;
	u32 count;
} irq_t;

static irq_t irqs[16];

void do_irq(regs_t *r)
{
	if (irqs[r->vector - 32].handler)
	{
		irq_t *irq = &irqs[r->vector - 32];
		(irq->handler)(irq->data);
		irq->count++;
		if (r->vector - 32 < 8) ack_master_pic();
		else ack_slave_pic();
	}
	else
	{
		printk("Unregistered IRQ #%d\n", r->vector - 32);
		BUG();
	}
}

int register_irq(int irq, irqhandler_t handler, void *data)
{
	u32 flags;
	if (irqs[irq].handler) return -1;
	flags = save_flags_irq();
	irqs[irq].data = data;
	irqs[irq].count = 0;
	irqs[irq].handler = handler;
	unmask_irq(irq);
	restore_flags(flags);
	return 0;
}

int release_irq(int irq)
{
	u32 flags;
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
	void scheduler_tick(void);
	scheduler_tick();
}
