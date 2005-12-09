#ifndef STDARG_H
#define STDARG_H

#include <stddef.h>

typedef char *va_list;

#define va_size(obj) (((sizeof(obj) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
#define va_start(ap, last) do {ap = (char *)&last + va_size(last);} while (0)
#define va_end(ap) do {ap = NULL;} while (0)
#define va_arg(ap, type) (*(type *)((ap += va_size(type)) - va_size(type)))

#endif
