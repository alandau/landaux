#include <string.h>

void *memcpy(void *dest, const void *src, u32 count)
{
	u64 tmp1, tmp2, tmp3;
	__asm__ __volatile__ (
		"rep\n\t"
		"movsb"
		: "=&c" (tmp1), "=&D" (tmp2), "=&S" (tmp3)
		: "0" (count), "1" (dest), "2" (src)
		: "memory");
	return dest;
}

void *memmove(void *dest, const void *src, u32 count)
{
	u64 tmp1, tmp2, tmp3;
	if (dest < src)
	{
		__asm__ __volatile__ (
			"rep\n\t"
			"movsb"
			: "=&c" (tmp1), "=&D" (tmp2), "=&S" (tmp3)
			: "0" (count), "1" (dest), "2" (src)
			: "memory");
	}
	else
	{
		__asm__ __volatile__ (
			"std\n\t"
			"rep\n\t"
			"movsb\n\t"
			"cld"
			: "=&c" (tmp1), "=&D" (tmp2), "=&S" (tmp3)
			: "0" (count), "1" ((char *)dest + count-1), "2" ((const char *)src + count-1)
			: "memory");
	}
	return dest;
}

void *memset(void *dest, int value, u32 count)
{
	u64 tmp1, tmp2;
	__asm__ __volatile__ (
		"rep\n\t"
		"stosb"
		: "=&c" (tmp1), "=&D" (tmp2)
		: "0" (count), "1" (dest), "a" (value)
		: "memory");
	return dest;
}

void *memset16(void *dest, u16 value, u32 count)
{
	u64 tmp1, tmp2;
	__asm__ __volatile__ (
		"rep\n\t"
		"stosw"
		: "=&c" (tmp1), "=&D" (tmp2)
		: "0" (count), "1" (dest), "a" (value)
		: "memory");
	return dest;
}

void *memset32(void *dest, u32 value, u32 count)
{
	u64 tmp1, tmp2;
	__asm__ __volatile__ (
		"rep\n\t"
		"stosl"
		: "=&c" (tmp1), "=&D" (tmp2)
		: "0" (count), "1" (dest), "a" (value)
		: "memory");
	return dest;
}

void *strcpy(char *dest, const char *src)
{
	u64 tmp1, tmp2, tmp3;
	__asm__ __volatile__ (
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al, %%al\n\t"
		"jnz 1b"
		: "=&S" (tmp1), "=&D" (tmp2), "=&a" (tmp3)
		: "0" (src), "1" (dest)
		: "memory");
	return dest;
}

u32 strlen(const char *s)
{
	int tmp;
	register unsigned long res;
	__asm__ __volatile__ (
		"repnz\n\t"
		"scasb\n\t"
		"not %0\n\t"
		"dec %0\n\t"
		"movl %%ecx, %%ecx"
		: "=c" (res), "=&D" (tmp)
		: "0" (0xFFFFFFFF), "1" (s), "a" (0));
	return res;
}
