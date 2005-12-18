#include <mm.h>

void init(void)
{
	while (1);
	__asm__ __volatile__ ("int $0x30");
}

void init_ends(void)
{
}
