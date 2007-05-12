#include <block.h>
#include <kernel.h>

static int floppy_init(blockdev_t *dev)
{
	return 0;
}

static u32 floppy_read(blockdev_t *dev, u32 offset, void *buf, u32 count)
{
	return 0;
}

static blockdev_ops_t floppy_ops = {
	.init = floppy_init,
	.read = floppy_read
};

int init_floppy(void)
{
	return register_block(&floppy_ops);
}
