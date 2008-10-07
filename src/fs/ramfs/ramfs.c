#include <fs.h>
#include <kernel.h>

static int ramfs_mount(superblock_t *sb)
{
	dentry_t *d;
	inode_t *i;
       	d = kzalloc(sizeof(dentry_t));
	if (d == NULL)
		return -ENOMEM;
	i = kzalloc(sizeof(inode_t));
	if (i == NULL) {
		kfree(d);
		return -ENOMEM;
	}
	sb->root_dentry = dentry_get(d);
	strcpy(d->name, "/");
	d->parent = NULL;
	d->sb = sb;
	d->inode = inode_get(i);
	i->id = 1;
	return 0;
}

static dentry_t *make_dentry(superblock_t *sb, int i, char *name)
{
	dentry_t *d = kzalloc(sizeof(dentry_t));
	dentry_get(d);
	strcpy(d->name, name);
	d->sb = sb;
	d->inode = inode_get(kzalloc(sizeof(inode_t)));
	d->inode->id= i;
	return d;
}

static int ramfs_lookup(superblock_t *sb, inode_t *i, char *name, dentry_t **d)
{
	switch (i->id) {
	case 1:
		*d = make_dentry(sb, 2, "a");
		return 0;
	}
	return -ENOENT;
}

static fs_t ramfs_fs = {
	.name = "ramfs",
	.mount = ramfs_mount,
	.lookup = ramfs_lookup,
};

int init_ramfs(void)
{
	return register_fs(&ramfs_fs);
}
