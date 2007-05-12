#ifndef BLOCK_H
#define BLOCK_H

#include <stddef.h>

struct blockdev_ops_struct;

typedef struct blockdev_struct
{
	int dev;
	u32 size;
	struct blockdev_ops_struct *ops;
} blockdev_t;

typedef struct blockdev_ops_struct
{
	int (*init)(blockdev_t *dev);
	u32 (*read)(blockdev_t *dev, u32 offset, void *buf, u32 count);
	u32 (*write)(blockdev_t *dev, u32 offset, const void *buf, u32 count);
} blockdev_ops_t;

void init_block(void);
int register_block(blockdev_ops_t *ops);
u32 block_read(int dev, u32 offset, void *buf, u32 count);
u32 block_write(int dev, u32 offset, const void *buf, u32 count);


#endif
