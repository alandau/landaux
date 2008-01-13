#ifndef KERNEL_H
#define KERNEL_H

#include <arch.h>
#include <stddef.h>

int printk(const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);

#define BUG()		do {printk("BUG() in %s at line %d\n", __FILE__, __LINE__); \
				print_stack(NULL); cli(); halt();} while (0)
#define BUG_ON(x)	do {if (!!(x)) BUG();} while (0)

void dump_regs(const regs_t *r);
void oops(const regs_t *r);
void print_stack(const regs_t *r);

#endif
