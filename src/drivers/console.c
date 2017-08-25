#include <kernel.h>
#include <devfs.h>
#include <console.h>

int console_open(file_t *f, int flags) {
	printk("in console_open\n");
	return 0;
}

int console_close(file_t *f) {
	printk("in console_close\n");
	return 0;
}

int console_read(file_t *f, u32 offset, char *buf, u32 size) {
	printk("in console_read\n");
	return 0;
}

int console_write(file_t *f, u32 offset, const char *buf, u32 size) {
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
	printk("haha from console driver\n");
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
