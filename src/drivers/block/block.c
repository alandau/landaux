#include <block.h>

#define MAXDEV 5

blockdev_t devices[MAXDEV];

u32 block_read(int dev, u32 offset, void *buf, u32 count)
{
	if (dev < 0 || dev >= MAXDEV) return -1;
	if (devices[dev].ops->write == NULL) return -1;
	return devices[dev].ops->write(&devices[dev], offset, buf, count);
}

u32 block_write(int dev, u32 offset, const void *buf, u32 count)
{
	if (dev < 0 || dev >= MAXDEV) return -1;
	if (devices[dev].ops->write == NULL) return -1;
	return devices[dev].ops->write(&devices[dev], offset, buf, count);
}

void init_block(void)
{
	int i;
	for (i=0; i<MAXDEV; i++)
		devices[i].dev = -1;
}

int register_block(blockdev_ops_t *ops)
{
	if (!ops) return -1;
	int i;
	for (i=0; i<MAXDEV; i++)
		if (devices[i].dev == -1)
		{
			devices[i].dev = i;
			devices[i].ops = ops;
			if (devices[i].ops->init)
				if (devices[i].ops->init(&devices[i]) < 0)
				{
					devices[i].dev = -1;
					return -1;
				}
			return i;
		}
	return -1;
}
