#ifndef IRQ_H
#define IRQ_H

typedef void (*irqhandler_t)(void *data);

int register_irq(int irq, irqhandler_t handler, void *data);
int release_irq(int irq);

#endif
