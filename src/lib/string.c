#include <string.h>
#include <kernel.h>

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

char *strchr(const char *str, char c)
{
	for (; *str; str++) {
		if (*str == c)
			return (char *)str;
	}
	return NULL;
}

char *strrchr(const char *str, char c)
{
	char *p;
	for (p = (char *)str + strlen(str); p != str - 1; p--) {
		if (*p == c)
			return p;
	}
	return NULL;
}

char *strdup(const char *s)
{
	int len = strlen(s) + 1;
	char *p = kmalloc(len);
	if (!p)
		return NULL;
	return memcpy(p, s, len);
}

char *basename(char *s)
{
	BUG_ON(*s == '\0');
	char *p = s + strlen(s) - 1;
	while (p != s && *p == '/')
		*p-- = '\0';
	char *slash = strrchr(s, '/');
	if (slash == NULL)
		return s;
	return slash + 1;
}

char *dirname(char *s)
{
	char *p = basename(s);
	if (p == s)
		return "";
	p--;
	if (p == s)
		return "/";
	*p = '\0';
//	while (p != s && *p == '/')
//		*p-- = '\0';
	return s;
}
