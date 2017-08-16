#ifndef STDARG_H
#define STDARG_H

#include <stddef.h>

typedef __builtin_va_list va_list;

#define va_start(ap, arg) __builtin_va_start(ap, arg)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

#endif
