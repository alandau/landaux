#include <arch.h>
#include <console.h>
#include <stdarg.h>
#include <string.h>
#include <mem.h>

/* This is only legal since the text framebuffer is below 4MB */
#define P2V(addr)		((addr) + KERNEL_VIRT_ADDR)

static u16 cursor = 0;
static char *screen = (char *)P2V(0xB8000);
static u8 color = 0x07;

void init_console(void)
{
	u8 tmp;
	outb(0x3D4, 0x0F);
	tmp = inb(0x3D5);
	outb(0x3D4, 0x0E);
	cursor = (inb(0x3D5) << 8) + tmp;
}

static void move_cursor(void)
{
	/* cursor LOW port to vga INDEX register */
	outb(0x3D4, 0x0F);
	outb(0x3D5, cursor & 0xFF);

	/* cursor HIGH port to vga INDEX register */
	outb(0x3D4, 0x0E);
	outb(0x3D5, cursor >> 8);
}

static void scroll(void)
{
	if (cursor / COLUMNS >= ROWS) {
		cursor -= COLUMNS;
		/* scroll everything 1 line up */
		memmove(screen, screen + COLUMNS*2, (ROWS-1) * COLUMNS * 2);
		/* clear the last line */
		memset16(screen + (ROWS-1) * COLUMNS * 2, (color << 8) + ' ', COLUMNS);
	}
}

void console_putc(char c)
{
	int tmp;
	switch (c) {
	case '\n':
		cursor = ((cursor + COLUMNS) / COLUMNS) * COLUMNS;
		break;
	case '\r':
		cursor = (cursor / COLUMNS) * COLUMNS;
		break;
	case '\t':
		tmp = ((cursor + 7) / 8) * 8 - cursor;
		cursor += tmp;
		memset16(&screen[cursor * 2], (color << 8) + ' ', tmp);
		break;
	default:
		screen[cursor * 2] = c;
		screen[cursor * 2 + 1] = color;
		cursor++;
		break;
	}
	scroll();
	move_cursor();
}

void console_puts(const char *s)
{
	for (; *s; s++)
		console_putc(*s);

}
