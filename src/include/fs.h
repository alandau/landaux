#ifndef FS_H
#define FS_H

#define PATH_MAX 255

typedef struct superblock {
	struct fs *fs;
	struct dentry *root_dentry;
	void *fs_priv;
} superblock_t;

typedef struct inode {
	int id;
	int refcnt;
} inode_t;

typedef struct dentry {
	char name[PATH_MAX];
	inode_t *inode;
	struct dentry *parent;
	int refcnt;
	superblock_t *sb;	/* of the containing fs */
	superblock_t *mnt;	/* of the mounted fs on this mountpoint, or NULL if not mountpoint */
} dentry_t;

typedef struct fs {
	char *name;
	int (*mount)(superblock_t *sb);
	int (*lookup)(superblock_t *sb, inode_t *i, char *name, dentry_t **d);
} fs_t;

inode_t *inode_get(inode_t *i);
void inode_put(inode_t *i);
dentry_t *dentry_get(dentry_t *d);
void dentry_put(dentry_t *d);

int register_fs(fs_t *fs);
dentry_t *lookup_path(char *path);
int vfs_mount(char *fstype, char *path);

#endif
