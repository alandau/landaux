#ifndef FS_H
#define FS_H

#include <stddef.h>

#define PATH_MAX 255

typedef struct superblock {
	struct fs *fs;
	struct dentry *root_dentry;
	void *fs_priv;
} superblock_t;

#define DIR_FILE	1
#define DIR_DIR		2
//#define DIR_LINK	3
#define DIR_TYPE(x)	((x) & 7)

typedef struct dentry {
	char name[PATH_MAX];
	struct dentry *parent;
	int refcnt;
	superblock_t *sb;	/* of the containing fs */
	superblock_t *mnt;	/* of the mounted fs on this mountpoint, or NULL if not mountpoint */
	void *priv;
	u32 mode;
	u32 size;
} dentry_t;

typedef struct {
	u32 reclen;
	u32 mode;
	u32 size;
	char name[0];
} user_dentry_t;

typedef struct fs {
	char *name;
	int (*mount)(superblock_t *sb);
	int (*lookup)(dentry_t *d, char *name, dentry_t **new_d);
	int (*getdents)(dentry_t *d, void *buf, u32 size, int start);
} fs_t;

dentry_t *dentry_get(dentry_t *d);
void dentry_put(dentry_t *d);
int vfs_add_dentry(void **buffer, u32 *bufsize, char *name, u32 mode, u32 size);

int register_fs(fs_t *fs);
dentry_t *lookup_path(char *path);
int vfs_mount(char *fstype, char *path);
int vfs_getdents(dentry_t *d, void *buf, u32 size, int start);

#endif
