#ifndef VIDEO_H
#define VIDEO_H

#include <stdarg.h>

#define COLUMNS 80
#define ROWS 25

void init_video(void);
void gotoxy(int x, int y);
int vsprintf(char *buf, const char *format, va_list args);
int sprintf(char *buf, const char *format, ...);
void printk(const char *format, ...);

#endif
