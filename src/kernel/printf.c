/* Exclaim printf functions
 * Copyright (C) 2007 Alex Smith
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <console.h>

#include <stdarg.h>
#include <string.h>

/* Definitions. Don't ask me what they do, I forgot. */
#define PRINT_BUF_LEN	12
#define PAD_RIGHT	1
#define PAD_ZERO	2

/* Flags used in processing format string */
#define	PR_LJ		0x001		/* Left justify */
#define	PR_CA		0x002		/* Use A-F instead of a-f for hex */
#define	PR_SG		0x004		/* Signed numeric conversion (%d vs. %u) */
#define PR_64		0x100		/* long long (64-bit) numeric conversion */
#define	PR_32		0x008		/* long (32-bit) numeric conversion */
#define	PR_16		0x010		/* short (16-bit) numeric conversion */
#define	PR_WS		0x020		/* PR_SG set and num was < 0 */
#define	PR_LZ		0x040		/* Pad left with '0' instead of ' ' */
#define	PR_FP		0x080		/* Pointers are far */

/* largest number handled is 2^64-1, lowest radix handled is 8.
 * 2^64-1 in base 8 has 22 digits (add 5 for trailing NUL and for slop) */
#define	PR_BUFLEN	27

static int do_printf(const char *fmt, va_list args, int (*fn)(int c, void *ptr), void *ptr)
{
	unsigned flags, actual_wd, count, given_wd;
	unsigned char *where, buf[PR_BUFLEN];
	unsigned char state, radix;
	long long num;
	
	state = flags = count = given_wd = 0;

	/* Begin scanning format specifier list */
	for(; *fmt; fmt++) {
		switch(state) {
			/* State 0: Awaiting % */
			case 0:
				if(*fmt != '%')	{
					/* It's not %, just call helper */
					if(fn(*fmt, ptr) != 0)
						return count;

					count++;
					break;
				}

				/* Found %, get next char and advance state
				 * to check if next char is a flag */
				state++;
				fmt++;
				/* FALL THROUGH */
			/* State 1: Awaiting flags (%-0) */
			case 1:
				if(*fmt == '%') {
					/* %% */
					if(fn(*fmt, ptr) != 0)
						return count;

					count++;
					state = flags = given_wd = 0;
					break;
				} else if(*fmt == '-') {
					/* %-- is illegal */
					if(flags & PR_LJ)
						state = flags = given_wd = 0;
					else
						flags |= PR_LJ;

					break;
				}

				/* Not a flag char: advance state to check
				 * if it's field width */
				state++;

				/* Check now for '%0...' */
				if(*fmt == '0') {
					flags |= PR_LZ;
					fmt++;
				}
				/* FALL THROUGH */
			/* State 2: Awaiting (numeric) field width */
			case 2:
				if(*fmt >= '0' && *fmt <= '9') {
					given_wd = 10 * given_wd + (*fmt - '0');
					break;
				}

				/* Not field width: advance state to check
				 * if it's a modifier */
				state++;
				/* FALL THROUGH */
			/* State 3: Awaiting modifier chars (FNlh) */
			case 3:
				if(*fmt == 'F') {
					flags |= PR_FP;
					break;
				} else if(*fmt == 'N') {
					break;
				} else if(*fmt == 'l') {
					if(flags & PR_32) {
						flags |= PR_64;
						flags &= ~PR_32;
					} else if(!(flags & PR_64)) {
						flags |= PR_32;
					}

					break;
				} else if(*fmt == 'L') {
					flags |= PR_64;
					break;
				} else if(*fmt == 'h') {
					flags |= PR_16;
					break;
				}

				/* Not modifier: advance state to check
				 * if it's a conversion char */
				state++;
				/* FALL THROUGH */
			/* State 4: Awaiting conversion chars (Xxpndiuocs) */
			case 4:
				where = buf + PR_BUFLEN - 1;
				*where = '\0';

				switch(*fmt) {
				case 'X':
					flags |= PR_CA;
					/* FALL THROUGH */
				/* xxx - far pointers (%Fp, %Fn) not yet supported */
				case 'x':
				case 'p':
				case 'n':
					radix = 16;
					goto DO_NUM;
				case 'd':
				case 'i':
					flags |= PR_SG;
					/* FALL THROUGH */
				case 'u':
					radix = 10;
					goto DO_NUM;
				case 'o':
					radix = 8;

					/* Load the value to be printed. l=long=32 bits: */
DO_NUM:					if(flags & PR_32) {
						num = va_arg(args, unsigned long);
					} else if(flags & PR_64) {
						num = va_arg(args, unsigned long long);
					} else if(flags & PR_16) {
						/* h=short=16 bits (signed or unsigned) */
						if(flags & PR_SG)
							num = va_arg(args, int) & 0xFFFF;
						else
							num = va_arg(args, unsigned int) & 0xFFFF;
					} else {
						/* No h nor l: sizeof(int)
						 * bits (signed or unsigned) */
						if(flags & PR_SG)
							num = va_arg(args, int);
						else
							num = va_arg(args, unsigned int);
					}

					/* Take care of sign */
					if(flags & PR_SG) {
						if(num < 0) {
							flags |= PR_WS;
							num = -num;
						}
					}

					/* Convert binary to octal/decimal/hex ASCII */
					do {
						unsigned /*long*/ long temp;
						
						temp = (unsigned /*long*/ long)num % radix;
						where--;
						if(temp < 10)
							*where = temp + '0';
						else if(flags & PR_CA)
							*where = temp - 10 + 'A';
						else
							*where = temp - 10 + 'a';

						num = (unsigned /*long*/ long)num / radix;
					} while(num != 0);

					goto EMIT;
				case 'c':
					/* Disallow pad-left-with-zeroes for
					 * %c */
					flags &= ~PR_LZ;
					where--;
					*where = (unsigned char)va_arg(args,
						unsigned int) & 0xFF;
					actual_wd = 1;
					goto EMIT2;
				case 's':
					/* Disallow pad-left-with-zeroes for
					 * %s */
					flags &= ~PR_LZ;

					where = va_arg(args, unsigned char *);
					if(where == NULL) {
						strcpy((char *)buf, "(NULL)");
						where = buf;
					}
EMIT:	
					actual_wd = strlen((char *)where);
					if(flags & PR_WS)
						actual_wd++;

					/* If we pad left with ZEROES, do the
					 * sign now */
					if((flags & (PR_WS | PR_LZ)) == (PR_WS | PR_LZ)) {
						if(fn(*fmt, ptr) != 0)
							return count;

						count++;
					}

					/* Pad on left with spaces or zeroes
					 * (for right justify) */
EMIT2:					if((flags & PR_LJ) == 0) {
						while(given_wd > actual_wd) {
							if(fn(flags & PR_LZ ? '0' : ' ', ptr) != 0)
								return count;

							count++;
							given_wd--;
						}
					}

					/* If we pad left with SPACES, do the
					 * sign now */
					if((flags & (PR_WS | PR_LZ)) == PR_WS) {
						if(fn('-', ptr) != 0)
							return count;

						count++;
					}

					/* Emit string/char/converted number */
					while(*where != '\0') {
						if(fn(*where++, ptr) != 0)
							return count;

						count++;
					}

					/* Pad on right with spaces (for
					 * left justify) */
					if(given_wd < actual_wd)
						given_wd = 0;
					else
						given_wd -= actual_wd;

					for(; given_wd; given_wd--) {
						if(fn(' ', ptr) != 0)
							return count;

						count++;
					}

					break;
				default:
					break;
				}
			default:
				state = flags = given_wd = 0;
				break;
		}
	}
	return count;
}

static int printf_helper(int c, void *data)
{
	console_putc(c);
	return 0;
}

int printk(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = do_printf(format, args, printf_helper, NULL);
	va_end(args);

	return ret;
}

struct sprintf_data {
	char *str;

	size_t size;
	size_t off;
};

static int sprintf_helper(int c, void *_data)
{
	struct sprintf_data *data = _data;

	if(data->off < data->size)
		data->str[data->off] = c;

	data->off++;
	return 0;
}

static int vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	struct sprintf_data data;

	if(size == 0)
		return 0;

	data.str = str;
	data.size = size - 1;
	data.off = 0;

	do_printf(format, args, sprintf_helper, &data);

	if(data.off < data.size)
		data.str[data.off] = 0;
	else
		data.str[data.size-1] = 0;

	return data.off;
}

int sprintf(char *str, const char *format, ...)
{
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsnprintf(str, 0xFFFFFFFFu, format, args);
	va_end(args);

	return ret;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsnprintf(str, size, format, args);
	va_end(args);

	return ret;
}
