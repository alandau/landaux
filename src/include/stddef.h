#ifndef STDDEF_H
#define STDDEF_H

#define NULL ((void *)0)
#define STR(x) #x

typedef char s8;
typedef short s16;
typedef long s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

typedef unsigned long size_t;

#define ROUND_UP(x,y)	(((u32)(x) + (y) - 1) & ~ ((y) - 1))

#endif
