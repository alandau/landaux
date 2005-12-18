#ifndef KERNEL_H
#define KERNEL_H

#include <arch.h>
#include <video.h>
#include <stddef.h>

#define BUG() do {printk("BUG() in %s at line %d\n", __FILE__, __LINE__); \
		print_stack(NULL); cli(); while (1);} while (0)

void dump_regs(const regs_t *r);
void oops(const regs_t *r);
void print_stack(const regs_t *r);

#endif
