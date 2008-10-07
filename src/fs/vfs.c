#include <fs.h>
#include <kernel.h>

#define MAX_FS_COUNT 4

static fs_t *fs_list[MAX_FS_COUNT];
static int fs_count;
static superblock_t rootfs[1];

int register_fs(fs_t *fs)
{
	if (fs_count >= MAX_FS_COUNT)
		return -ENOMEM;
	fs_list[fs_count++] = fs;
	return 0;
}

static int find_fs(char *name)
{
	int i;
	for (i=0; i<fs_count; i++)
		if (strcmp(fs_list[i]->name, name) == 0)
			return i;
	return -1;
}

int vfs_mount(char *fstype, char *path)
{
	int fs = find_fs(fstype);
	if (fs == -1)
		return -ENOENT;
	memset(rootfs, 0, sizeof(superblock_t));
	rootfs->fs = fs_list[fs];
	return rootfs->fs->mount(rootfs);
}

static char *get_next_component(char **path)
{
	char *p = *path, *ret;
	if (*p == '\0')
		return NULL;
	while (*p && *p != '/')
		p++;
	ret = kmalloc(p - *path + 1);
	if (!ret)
		return NULL;
	memcpy(ret, *path, p - *path);
	ret[p - *path] = '\0';
	*path = p;
	return ret;
}

inode_t *inode_get(inode_t *i)
{
	i->refcnt++;
	return i;
}

void inode_put(inode_t *i)
{
	if (--i->refcnt == 0)
		kfree(i);
}

dentry_t *dentry_get(dentry_t *d)
{
	d->refcnt++;
	return d;
}

void dentry_put(dentry_t *d)
{
	if (--d->refcnt == 0) {
		inode_put(d->inode);
		if (d->parent)
			dentry_put(d->parent);
		printk("freeing dentry: %s\n", d->name);
		kfree(d);
	}
}

dentry_t *lookup_path(char *path)
{
	dentry_t *d = dentry_get(rootfs->root_dentry);
	BUG_ON(path[0] != '/');
	while (1) {
		char *name;
		dentry_t *new_dentry;
		int ret;
		while (*path == '/')
			path++;
		if (*path == '\0')
			break;
		name = get_next_component(&path);
		if (name == NULL) {
			dentry_put(d);
			return NULL;
		}
		if (d->mnt) {	/* is mountpoint */
			ret = d->mnt->fs->lookup(d->mnt, d->mnt->root_dentry->inode, name, &new_dentry);
		} else {
			ret = d->sb->fs->lookup(d->sb, d->inode, name, &new_dentry);
		}
		if (ret < 0) {
			kfree(name);
			dentry_put(d);
			return NULL;
		}
		new_dentry->parent = dentry_get(d);
		dentry_put(d);
		d = new_dentry;
		kfree(name);
	}
	return d;
}
