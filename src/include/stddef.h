#ifndef STDDEF_H
#define STDDEF_H

#define NULL ((void *)0)
#define STR(x) _STR(x)
#define _STR(x) #x

typedef char s8;
typedef short s16;
typedef int s32;
typedef long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef unsigned long size_t;

#define ROUND_UP(x,y)	(((u32)(x) + (y) - 1) & ~ ((y) - 1))

#endif
