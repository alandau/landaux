#pragma once

#include <fs.h>

#define init_function(f) static int (*init_function_ ## LINE)(void) __attribute__((section(".initfuncs"), used)) = f

typedef struct {
	int (*open)(file_t *f, int flags);
	int (*close)(file_t *f);
	int (*read)(file_t *f, u32 offset, char *buf, u32 size);
	int (*write)(file_t *f, u32 offset, const char *buf, u32 size);
} devfs_ops_t;

int devfs_register(const char *path, devfs_ops_t *ops);
