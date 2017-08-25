#include <kernel.h>
#include <devfs.h>
#include <console.h>
#include <sched.h>
#include <irq.h>

static const unsigned char table[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

#define BUFSIZE 256
static char buffer[BUFSIZE];
static u32 head, len;

static waitqueue_t q = WAITQUEUE_INIT(q);

static void keyboard_irq(void *data)
{
	u8 key = inb(0x60);
	if (key & 0x80) {
		// Release
		return;
	}
	if (len >= BUFSIZE) {
		return;
	}
	if (table[key] == 0) {
		return;
	}
	buffer[(head + len++) % BUFSIZE] = table[key];
	waitqueue_wakeup(&q);
}

static int console_open(file_t *f, int flags) {
	printk("in console_open\n");
	return 0;
}

static int console_close(file_t *f) {
	printk("in console_close\n");
	return 0;
}

static int console_read(file_t *f, u32 offset, char *buf, u32 size) {
	if (size == 0) {
		return 0;
	}

	while (len == 0) {
		waitqueue_sleep(&q);
		schedule();
	}

	u64 flags = save_flags_irq();
	int l = min(min(len, size), BUFSIZE - head);
	memcpy(buf, &buffer[head], l);
	head = (head + l) % BUFSIZE;
	len -= l;
	
	restore_flags(flags);
	return l;
}

static int console_write(file_t *f, u32 offset, const char *buf, u32 size) {
	for (int i = 0; i < size; i++) {
		console_putc(buf[i]);
	}
	return size;
}

static devfs_ops_t ops = {
	.open = console_open,
	.close = console_close,
	.read = console_read,
	.write = console_write,
};

static int init(void) {
	register_irq(1, keyboard_irq, NULL);
	int ret = devfs_register("platform/console", &ops);
	if (ret < 0) {
		printk("Error registering console driver: %d\n", ret);
		return ret;
	}

	void tree(char *path);
	tree("/");
	return 0;
}

init_function(init);
