#include <arch.h>
#include <video.h>
#include <stdarg.h>
#include <string.h>
#include <mm.h>

static u16 cursor = 0;
static char *screen = (char *)0xB8000;
static u8 color = 0x07;

void init_video(void)
{
	u8 tmp;
	outb(0x3D4, 0x0F);
	tmp = inb(0x3D5);
	outb(0x3D4, 0x0E);
	cursor = (inb(0x3D5) << 8) + tmp;
	screen = ioremap((u32)screen, ROWS*COLUMNS*2, 0, MAP_READWRITE);
}

u8 get_color(void)
{
	return color;
}

void set_color(u8 c)
{
	color = c;
}

void gotoxy(int x, int y)
{
	if (x < 0 || x >= COLUMNS || y < 0 || y >= ROWS) return;
	cursor = y * COLUMNS + x;

	/* cursor LOW port to vga INDEX register */
	outb(0x3D4, 0x0F);
	outb(0x3D5, cursor & 0xFF);

	/* cursor HIGH port to vga INDEX register */
	outb(0x3D4, 0x0E);
	outb(0x3D5, cursor >> 8);
}

static void scroll(void)
{
	/* scroll everything 1 line up */
	memmove(screen, screen + COLUMNS*2, (ROWS-1) * COLUMNS * 2);
	/* clear the last line */
	memset16(screen + (ROWS-1) * COLUMNS * 2, (color << 8) + ' ', COLUMNS);
}

static void print_str(const char *s)
{
	int tmp;
	for (; *s; s++)
	{
		switch (*s)
		{
		case '\n':
			cursor = ((cursor + COLUMNS) / COLUMNS) * COLUMNS;
			if (cursor / COLUMNS >= ROWS)
			{
				cursor -= COLUMNS;
				scroll();
			}
			break;
		case '\r':
			cursor = (cursor / COLUMNS) * COLUMNS;
			break;
		case '\t':
			tmp = ((cursor + 7) / 8) * 8 - cursor;
			cursor += tmp;
			if (cursor / COLUMNS >= ROWS)
			{
				cursor -= COLUMNS;
				scroll();
			}
			memset16(&screen[cursor * 2], (color << 8) + ' ', tmp);
			break;
		default:
			screen[cursor * 2] = *s;
			screen[cursor * 2 + 1] = color;
			cursor++;
			if (cursor / COLUMNS >= ROWS)
			{
				cursor -= COLUMNS;
				scroll();
			}
			break;
		}
	}
}

static int sprintf_uint(char *buf, u32 value)
{
#define ROUND_LONG 1000000000UL
	u32 maxlong = ROUND_LONG;
	int count = 0, flag = 0;
	if (value == 0)
	{
		*(buf++) = '0';
		return 1;
	}
	while (maxlong)
	{
		u32 c = value / maxlong;
		if (flag || c)
		{
			flag = 1;
			*(buf++) = c + '0';
			count++;
			value -= maxlong*c;
		}
		maxlong /= 10;
	}
	return count;
#undef ROUND_LONG
}

static int sprintf_hex(char *buf, u32 value, int small, int leading_zero)
{
#define ROUND_LONG 0x10000000UL
	u32 maxlong = ROUND_LONG;
	int count = 0;
	char hex[2][17] = {{"0123456789ABCDEF"}, {"0123456789abcdef"}};
	if (value == 0)
	{
		if (leading_zero)
		{
			memset(buf, '0', 8);
			return 8;
		}
		else
		{
			*(buf++) = '0';
			return 1;
		}
	}
	while (maxlong)
	{
		u32 c = value / maxlong;
		if (leading_zero || c)
		{
			leading_zero = 1;
			*(buf++) = hex[small][c];
			count++;
			value -= maxlong*c;
		}
		maxlong /= 16;
	}
	*buf = '\0';
	return count;
#undef ROUND_LONG
}

int vsprintf(char *buf, const char *format, va_list args)
{
	int count = 0, len, leading_zero;
	long l;
	char c, flag, *s, *olds;

	while ((c = *(format++)) != '\0')
	{
		if (c == '%')
		{
			flag = 0;
			leading_zero = 0;
			c = *(format++);
			if (c == '0')
			{
				leading_zero = 1;
				c = *(format++);
			}
			switch (c)
			{
			case 'd':
				flag = 1;	/* signed */
			case 'u':
				l = va_arg(args, long);
				if (flag && l < 0)
				{
					*(buf++) = '-';
					count++;
					l = -l;
				}
				len = sprintf_uint(buf, l);
				buf += len;
				count += len;
				break;
			case 'x':
				flag = 1;	/* small */
			case 'X':
				l = va_arg(args, long);
				len = sprintf_hex(buf, l, flag, leading_zero);
				buf += len;
				count += len;
				break;
			case 'c':
				c = va_arg(args, char);
				*(buf++) = c;
				count++;
				break;
			case 's':
				olds = s = va_arg(args, char *);
				for (; *s; *(buf++) = *(s++)) /* nothing */;
				count += (s - olds);
				break;
			default:
				*(buf++) = c;
				count++;
				break;
			}
		}
		else
		{
			*(buf++) = c;
			count++;
		}
	}
	*buf = '\0';
	return count;
}

int sprintf(char *buf, const char *format, ...)
{
	va_list args;
	int count;
	va_start(args, format);
	count = vsprintf(buf, format, args);
	va_end(args);
	return count;
}

void printk(const char *format, ...)
{
	char buf[256];
	va_list args;
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	print_str(buf);
	gotoxy(cursor % COLUMNS, cursor / COLUMNS);
}
