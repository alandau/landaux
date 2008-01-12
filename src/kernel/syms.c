#include <syms.h>

extern symbol_t __ksymtab[];

/* '-1' because last entry of __ksymtab is {0,'',""} */
extern char _ksymtab[], _ksymtab_end[];
#define NUM_SYMS ((_ksymtab_end-_ksymtab) / sizeof(symbol_t) - 1)

/*
searches for the symbol with the maximum address <= 'address'. *sym will point to that entry.
returns 1 if found exact match, 0 otherwise
*/
symbol_t *find_symbol(unsigned long address)
{
	int i;
	for (i=0; i < NUM_SYMS; i++)
		if (__ksymtab[i].address >= address) break;
	if (i == NUM_SYMS || __ksymtab[i].address > address)
		return &__ksymtab[i-1];
	else
		return &__ksymtab[i];
}
