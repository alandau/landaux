#ifndef STRING_H
#define STRING_H

#define __always_inline inline __attribute__((always_inline))

static __always_inline void *memcpy(void *dest, const void *src, u32 count)
{
	u32 tmp1, tmp2, tmp3;
	__asm__ __volatile__ (
		"cld\n\t"
		"rep\n\t"
		"movsb"
		: "=&c" (tmp1), "=&D" (tmp2), "=&S" (tmp3)
		: "0" (count), "1" (dest), "2" (src)
		: "memory");
	return dest;
}

static inline void *memmove(void *dest, const void *src, u32 count)
{
	u32 tmp1, tmp2, tmp3;
	if (dest < src)
	{
		__asm__ __volatile__ (
			"cld\n\t"
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
			"movsb"
			: "=&c" (tmp1), "=&D" (tmp2), "=&S" (tmp3)
			: "0" (count), "1" ((char *)dest + count-1), "2" ((const char *)src + count-1)
			: "memory");
	}
	return dest;
}

static __always_inline void *memset(void *dest, int value, u32 count)
{
	u32 tmp1, tmp2;
	__asm__ __volatile__ (
		"cld\n\t"
		"rep\n\t"
		"stosb"
		: "=&c" (tmp1), "=&D" (tmp2)
		: "0" (count), "1" (dest), "a" (value)
		: "memory");
	return dest;
}

static inline void *memset16(void *dest, u16 value, u32 count)
{
	u32 tmp1, tmp2;
	__asm__ __volatile__ (
		"cld\n\t"
		"rep\n\t"
		"stosw"
		: "=&c" (tmp1), "=&D" (tmp2)
		: "0" (count), "1" (dest), "a" (value)
		: "memory");
	return dest;
}

static inline void *memset32(void *dest, u32 value, u32 count)
{
	u32 tmp1, tmp2;
	__asm__ __volatile__ (
		"cld\n\t"
		"rep\n\t"
		"stosl"
		: "=&c" (tmp1), "=&D" (tmp2)
		: "0" (count), "1" (dest), "a" (value)
		: "memory");
	return dest;
}

static inline void *strcpy(char *dest, const char *src)
{
	int tmp1, tmp2, tmp3;
	__asm__ __volatile__ (
		"cld\n\t"
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al, %%al\n\t"
		"jnz 1b"
		: "=&S" (tmp1), "=&D" (tmp2), "=&a" (tmp3)
		: "0" (src), "1" (dest)
		: "memory");
	return dest;
}

static inline unsigned long strlen(const char *s)
{
	int tmp;
	register unsigned long res;
	__asm__ __volatile__ (
		"cld\n\t"
		"repnz\n\t"
		"scasb\n\t"
		"notl %0\n\t"
		"decl %0"
		: "=c" (res), "=&D" (tmp)
		: "0" (0xFFFFFFFF), "1" (s), "a" (0));
	return res;
}

static inline void *strcat(char *dest, const char *src)
{
	strcpy(dest + strlen(dest), src);
	return dest;
}

#endif
