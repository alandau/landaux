#ifndef KERNEL_H
#define KERNEL_H

#include <arch.h>
#include <video.h>

#define BUG() do {printk("BUG() in %s at line %d\n", __FILE__, __LINE__); \
		print_stack(); cli(); while (1);} while (0)

void dump(const regs_t *r);
void oops(const regs_t *r);
void print_stack(void);

#endif
