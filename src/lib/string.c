#include <string.h>

int strcmp(const char *dest, const char *src)
{
	while (*dest) {
		int c = *dest - *src;
		if (c)
			return c;
		dest++;
		src++;
	}
	return 0;
}

char *strcat(char *dest, const char *src)
{
	strcpy(dest + strlen(dest), src);
	return dest;
}
