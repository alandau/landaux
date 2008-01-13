#ifndef VIDEO_H
#define VIDEO_H

#include <stdarg.h>

#define COLUMNS 80
#define ROWS 25

void init_console(void);
void console_putc(char c);
void console_puts(const char *c);

#endif
