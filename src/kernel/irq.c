#include <stddef.h>
#include <arch.h>
#include <irq.h>
#include <kernel.h>

volatile u32 jiffies;

typedef struct
{
	irqhandler_t handler;
	void *data;
	u32 count;
} irq_t;

static irq_t irqs[16];

void do_irq(regs_t r)
{
	if (irqs[r.intnum].handler)
	{
		irq_t *irq = &irqs[r.intnum];
		(irq->handler)(irq->data);
		irq->count++;
		if (r.intnum < 8) ack_master_pic();
		else ack_slave_pic();
	}
	else
	{
		printk("Unregistered IRQ #%d\n", r.intnum);
		BUG();
	}
}

int register_irq(int irq, irqhandler_t handler, void *data)
{
	u32 flags;
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

void do_keyboard(void *data)
{
	u8 key = inb(0x60);
//	printk("keyboard: %s - %d\n", key&0x80?"RELEASED":"PRESSED ",key&0x7F);
	if (key & 0x80) {
		u32 get_used_mem();
		static u32 size = 1;
		void *p = kmalloc(size);
		printk("Allocated %d bytes at %x, memused=%d\n", size, p, get_used_mem());
		kfree(p);
		printk("Freed %d bytes at %x, memused=%d\n", size, p, get_used_mem());
		size *= 2;
	}
}
