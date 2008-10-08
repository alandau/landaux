#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memcpy(void *dest, const void *src, u32 count);
void *memmove(void *dest, const void *src, u32 count);
void *memset(void *dest, int value, u32 count);
void *memset16(void *dest, u16 value, u32 count);
void *memset32(void *dest, u32 value, u32 count);
void *strcpy(char *dest, const char *src);
int strcmp(const char *dest, const char *src);
unsigned long strlen(const char *s);
char *strcat(char *dest, const char *src);

#endif
