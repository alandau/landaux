#ifndef SYMS_H
#define SYMS_H

#define MAX_SYMBOL_LEN		30

typedef struct symbol_struct
{
	unsigned long address;
	char type;
	char symbol[MAX_SYMBOL_LEN];
} __attribute__ ((packed)) symbol_t;

symbol_t *find_symbol(unsigned long address);

#endif
