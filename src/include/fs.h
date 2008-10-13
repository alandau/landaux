#ifndef FS_H
#define FS_H

#include <stddef.h>

#define O_RDONLY	1
#define O_WRONLY	2
#define O_RDWR		3

#define O_APPEND	4
#define O_CREAT		8
#define O_EXCL		16
#define O_TRUNC		32

#define PATH_MAX 255

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

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

typedef struct {
	dentry_t *dentry;
	int openmode;
	u32 offset;
	void *priv;
} file_t;

typedef struct fs {
	char *name;
	int (*mount)(superblock_t *sb);
	dentry_t *(*lookup)(dentry_t *d, const char *name);
	int (*getdents)(dentry_t *d, void *buf, u32 size, int start);
	int (*mkdir)(dentry_t *d, const char *name);
	int (*rmdir)(dentry_t *d);
	int (*create)(dentry_t *d, const char *name);
	int (*unlink)(dentry_t *d);
	int (*open)(file_t *f, int flags);
	int (*truncate)(file_t *f);
	int (*close)(file_t *f);
	int (*read)(file_t *f, u32 offset, char *buf, u32 size);
	int (*write)(file_t *f, u32 offset, const char *buf, u32 size);
	int (*lseek)(file_t *f, u32 offset, int whence);
} fs_t;

dentry_t *dentry_get(dentry_t *d);
void dentry_put(dentry_t *d);
int vfs_add_dentry(void **buffer, u32 *bufsize, char *name, u32 mode, u32 size);

int register_fs(fs_t *fs);
dentry_t *lookup_path(const char *path);
dentry_t *lookup_path_dir(const char *path);
dentry_t *lookup_path_for_create_file(const char *path, char **last_name);
dentry_t *lookup_path_for_create_dir(const char *path, char **last_name);
int vfs_mount(char *fstype, char *path);
int vfs_dgetdents(dentry_t *d, void *buf, u32 size, int start);
int vfs_dmkdir(dentry_t *d, char *name);
int vfs_drmdir(dentry_t *d);
int vfs_dunlink(dentry_t *d);
file_t *vfs_dopen(dentry_t *d, const char *name, int flags);
int vfs_close(file_t *f);
int vfs_getdents(const char *path, void *buf, u32 size, int start);
int vfs_mkdir(const char *path);
int vfs_rmdir(const char *path);
int vfs_unlink(const char *path);
file_t *vfs_open(const char *name, int flags);
int vfs_read(file_t *f, char *buf, u32 size);
int vfs_write(file_t *f, const char *buf, u32 size);
int vfs_lseek(file_t *f, u32 offset, int whence);

#endif
