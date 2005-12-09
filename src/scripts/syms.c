/*
usage: syms < system.map > symtable.c

creates .c file from symbol table to be included in the kernel
*/

#include <stdio.h>
#include "../include/syms.h"

#define MAX_LINE	80

int main(int argc, void *argv[])
{
	char line[MAX_LINE];
	symbol_t sym;

	printf("#include <syms.h>\n\n");
	printf("symbol_t __ksymtab[] __attribute__ ((section (\".ksymtab\")))={\n");
	while (scanf("%X %c %s", &sym.address, &sym.type, sym.symbol) != EOF)
	{
		printf("\t{0x%08X, '%c', \"%s\"},\n", sym.address, sym.type, sym.symbol);
	}
	printf("\t{0, 0, \"\"}\n");
	printf("};\n");

	return 0;
}
