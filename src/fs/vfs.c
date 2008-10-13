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
		return -ENODEV;
	if (rootfs->root_dentry == NULL) {
		if (strcmp(path, "/") != 0)
			return -ENODEV;
		memset(rootfs, 0, sizeof(superblock_t));
		rootfs->fs = fs_list[fs];
		return rootfs->fs->mount(rootfs);
	}
	dentry_t *d = lookup_path(path);
	if (!d)
		return -ENOENT;
	if (DIR_TYPE(d->mode) != DIR_DIR) {
		dentry_put(d);
		return -ENOTDIR;
	}
	if (d->mnt != NULL) {
		dentry_put(d);
		return -EBUSY;
	}
	superblock_t *sb = kzalloc(sizeof(superblock_t));
	if (!sb) {
		dentry_put(d);
		return -ENOMEM;
	}
	sb->fs = fs_list[fs];
	int ret = sb->fs->mount(sb);
	if (ret < 0) {
		dentry_put(d);
		kfree(sb);
		return ret;
	}
	d->mnt = sb;
	return 0;
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

dentry_t *dentry_get(dentry_t *d)
{
	d->refcnt++;
	return d;
}

void dentry_put(dentry_t *d)
{
	if (--d->refcnt == 0) {
		if (d->parent)
			dentry_put(d->parent);
		kfree(d->priv);
		kfree(d);
	}
}

dentry_t *lookup_path(const char *path)
{
	dentry_t *d = dentry_get(rootfs->root_dentry);
	BUG_ON(path[0] != '/');
	while (1) {
		char *name;
		dentry_t *new_dentry;
		while (*path == '/')
			path++;
		if (*path == '\0')
			break;
		name = get_next_component((char **)&path);
		if (name == NULL) {
			dentry_put(d);
			return ERR_PTR(-ENOMEM);
		}
		if (d->mnt) {	/* is mountpoint */
			new_dentry = d->mnt->fs->lookup(d->mnt->root_dentry, name);
		} else {
			new_dentry = d->sb->fs->lookup(d, name);
		}
		if (IS_ERR(new_dentry)) {
			kfree(name);
			dentry_put(d);
			return new_dentry;
		}
		new_dentry->parent = d;
		d = new_dentry;
		kfree(name);
	}
	return d;
}

int vfs_add_dentry(void **buffer, u32 *bufsize, char *name, u32 mode, u32 size)
{
	user_dentry_t **d = (user_dentry_t **)buffer;
	int len = strlen(name);
	int totallen = ((sizeof(user_dentry_t) + len + 1) + 3) & ~3;	/* round up */
	if (*bufsize < totallen)
		return 0;
	(*d)->reclen = totallen;	
	(*d)->mode = mode;
	(*d)->size = size;
	memcpy((*d)->name, name, len + 1);
	*bufsize -= totallen;
	*d = (user_dentry_t *)((char *)*d + totallen);
	return 1;
}

/* 
 * Fills buf of size bytes with dentries under directory d, starting with #start.
 * Returns the number of dentries filled, 0 for end, <0 for error.
 */
int vfs_dgetdents(dentry_t *d, void *buf, u32 size, int start)
{
	int ret;
	if (d->mnt) {	/* is mountpoint */
		ret = d->mnt->fs->getdents(d->mnt->root_dentry, buf, size, start);
	} else {
		ret = d->sb->fs->getdents(d, buf, size, start);
	}
	return ret;
}

int vfs_dmkdir(dentry_t *d, char *name)
{
	int ret;
	BUG_ON(strchr(name, '/') != NULL);
	if (DIR_TYPE(d->mode) != DIR_DIR)
		return -ENOTDIR;
	if (d->mnt) {	/* is mountpoint */
		ret = d->mnt->fs->mkdir(d->mnt->root_dentry, name);
	} else {
		ret = d->sb->fs->mkdir(d, name);
	}
	return ret;
}

int vfs_drmdir(dentry_t *d)
{
	if (DIR_TYPE(d->mode) != DIR_DIR)
		return -ENOTDIR;
	if (d->mnt || !d->parent) 	/* is mountpoint */
		return -EBUSY;
	return d->sb->fs->rmdir(d);
}

int vfs_getdents(const char *path, void *buf, u32 size, int start)
{
	int ret;
	dentry_t *d = lookup_path(path);
	if (IS_ERR(d))
		return PTR_ERR(d);
	ret = vfs_dgetdents(d, buf, size, start);
	dentry_put(d);
	return ret;
}

int vfs_mkdir(const char *path)
{
	int ret;
	char *p = strdup(path);
	if (!p)
		return -ENOMEM;
	char *base = basename(p);
	char *dir = dirname(p);
	dentry_t *d = lookup_path(dir);
	if (IS_ERR(d))
		return PTR_ERR(d);
	ret = vfs_dmkdir(d, base);
	dentry_put(d);
	kfree(p);
	return ret;
}

int vfs_rmdir(const char *path)
{
	int ret;
	dentry_t *d = lookup_path(path);
	if (IS_ERR(d))
		return PTR_ERR(d);
	ret = vfs_drmdir(d);
	dentry_put(d);
	return ret;
}
