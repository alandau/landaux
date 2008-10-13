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
	dentry_t *d = lookup_path_dir(path);
	if (IS_ERR(d))
		return PTR_ERR(d);
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

enum {COMP_NO_MORE, COMP_NORM, COMP_LAST, COMP_LAST_SLASH};

static int get_next_component(char **path, char **name)
{
	char *p = *path, *buf;
	int ret;
	if (*p == '\0')
		return COMP_NO_MORE;
	while (*p && *p != '/')
		p++;
	buf = kmalloc(p - *path + 1);
	if (!buf)
		return COMP_NO_MORE;
	memcpy(buf, *path, p - *path);
	buf[p - *path] = '\0';
	*name = buf;
	ret = COMP_NORM;
	if (!*p)
		ret = COMP_LAST;
	else if (*p == '/') {
		while (*p == '/')
			p++;
		if (!*p)
			ret = COMP_LAST_SLASH;
	} else
		BUG();
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

enum {LAST_MUST_NOT_EXIST=1, LAST_MUST_BE_DIR=2, LAST_NO_SLASHES=4, LAST_RETURN=8};

static dentry_t *lookup_path_common(const char *path, int last_flags, char **last_name)
{
	dentry_t *d = dentry_get(rootfs->root_dentry);
	char *name;
	BUG_ON(path[0] != '/');
	while (*path == '/')
		path++;
	if (*path == '\0')
		return d;
	while (1) {
		int type;
		dentry_t *new_dentry;
		type = get_next_component((char **)&path, &name);
		if (type == COMP_NO_MORE) {
			dentry_put(d);
			return ERR_PTR(-ENOMEM);
		}
		if ((type == COMP_LAST || (type == COMP_LAST_SLASH && (last_flags & ~LAST_NO_SLASHES))) &&
			(last_flags & LAST_RETURN))
			break;
		if (d->mnt) {	/* is mountpoint */
			new_dentry = d->mnt->fs->lookup(d->mnt->root_dentry, name);
		} else {
			new_dentry = d->sb->fs->lookup(d, name);
		}
		if (IS_ERR(new_dentry)) {
			if (PTR_ERR(new_dentry) == -ENOENT &&
				(type == COMP_LAST || (type == COMP_LAST_SLASH && (last_flags & ~LAST_NO_SLASHES))) &&
				(last_flags & LAST_MUST_NOT_EXIST))
				break;
			kfree(name);
			dentry_put(d);
			return new_dentry;
		}
		new_dentry->parent = d;
		d = new_dentry;
		if ((type == COMP_LAST || (type == COMP_LAST_SLASH && (last_flags & ~LAST_NO_SLASHES))) &&
			(last_flags & LAST_MUST_NOT_EXIST)) {
				kfree(name);
				dentry_put(d);
				return ERR_PTR(-ENOENT);
		}
		if (DIR_TYPE(d->mode) != DIR_DIR) {
			if (type == COMP_NORM || type == COMP_LAST_SLASH || (last_flags & LAST_MUST_BE_DIR)) {
				kfree(name);
				dentry_put(d);
				return ERR_PTR(-ENOTDIR);
			}
			break;
		}
		/* should check for LAST_NO_SLASHES? */
		if (type == COMP_LAST || type == COMP_LAST_SLASH)
			break;
		kfree(name);
	}
	if (last_name)
		*last_name = name;
	else
		kfree(name);
	return d;
}

dentry_t *lookup_path(const char *path)
{
	return lookup_path_common(path, 0, NULL);
}

dentry_t *lookup_path_dir(const char *path)
{
	return lookup_path_common(path, LAST_MUST_BE_DIR, NULL);
}

dentry_t *lookup_path_for_create_file(const char *path, char **last_name)
{
	return lookup_path_common(path, LAST_MUST_NOT_EXIST | LAST_NO_SLASHES, last_name);
}

dentry_t *lookup_path_for_create_dir(const char *path, char **last_name)
{
	return lookup_path_common(path, LAST_MUST_NOT_EXIST, last_name);
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
	dentry_t *d = lookup_path_dir(path);
	if (IS_ERR(d))
		return PTR_ERR(d);
	ret = vfs_dgetdents(d, buf, size, start);
	dentry_put(d);
	return ret;
}

int vfs_mkdir(const char *path)
{
	int ret;
	char *name;
	dentry_t *d = lookup_path_for_create_dir(path, &name);
	if (IS_ERR(d))
		return PTR_ERR(d);
	ret = vfs_dmkdir(d, name);
	dentry_put(d);
	kfree(name);
	return ret;
}

int vfs_rmdir(const char *path)
{
	int ret;
	dentry_t *d = lookup_path_dir(path);
	if (IS_ERR(d))
		return PTR_ERR(d);
	ret = vfs_drmdir(d);
	dentry_put(d);
	return ret;
}

file_t *vfs_dopen(dentry_t *d, const char *name, int flags)
{
	file_t *f;
	dentry_t *fd;

	if (d->mnt)	/* is mountpoint */
		d = d->mnt->root_dentry;
	fd = d->sb->fs->lookup(d, name);
	if (flags & O_CREAT) {
		if (IS_ERR(fd)) {
			if (!d->sb->fs->create)
				return ERR_PTR(-ENOSYS);
			int ret = d->sb->fs->create(d, name);
			if (ret < 0)
				return ERR_PTR(ret);
			fd = d->sb->fs->lookup(d, name);
			if (IS_ERR(fd))
				return (void *)fd;
		} else {
			if (DIR_TYPE(fd->mode) == DIR_DIR) {
				dentry_put(fd);
				return ERR_PTR(-EISDIR);
			}
			if (flags & O_EXCL) {
				dentry_put(fd);
				return ERR_PTR(-EEXIST);
			}
		}
	} else {
		if (IS_ERR(fd))
			return (void *)fd;
		else {
			if (DIR_TYPE(fd->mode) == DIR_DIR) {
				dentry_put(fd);
				return ERR_PTR(-EISDIR);
			}
		}
	}
	f = kzalloc(sizeof(file_t));
	if (!f) {
		dentry_put(fd);
		return ERR_PTR(-ENOMEM);
	}
	f->dentry = fd;
	f->openmode = flags & O_RDWR;
	f->offset = (flags & O_APPEND) ? fd->size : 0;
	if (fd->sb->fs->open) {
		int ret = fd->sb->fs->open(f, flags);
		if (ret < 0) {
			kfree(f);
			dentry_put(fd);
			return ERR_PTR(ret);
		}
	}
	if ((flags & O_TRUNC) && fd->sb->fs->truncate) {
		int ret = fd->sb->fs->truncate(f);
		if (ret < 0) {
			if (fd->sb->fs->close)
				fd->sb->fs->close(f);
			kfree(f);
			dentry_put(fd);
			return ERR_PTR(ret);
		}
	}
	return f;
}

file_t *vfs_open(const char *path, int flags)
{
	char *name;
	file_t *ret;
	dentry_t *d = lookup_path_common(path, LAST_RETURN | LAST_NO_SLASHES, &name);
	if (IS_ERR(d))
		return (void *)d;
	ret = vfs_dopen(d, name, flags);
	kfree(name);
	dentry_put(d);
	return ret;
}

int vfs_close(file_t *f)
{
	int ret = 0;
	if (f->dentry->sb->fs->close)
		ret = f->dentry->sb->fs->close(f);
	dentry_put(f->dentry);
	kfree(f);
	return ret;
}

int vfs_read(file_t *f, char *buf, u32 size)
{
	if (!(f->openmode & O_RDONLY))
		return -EINVAL;
	if (!f->dentry->sb->fs->read)
		return -ENOSYS;
	if (size == 0)
		return 0;
	int count = f->dentry->sb->fs->read(f, f->offset, buf, size);
	if (count > 0)
		f->offset += count;
	return count;
}

int vfs_write(file_t *f, const char *buf, u32 size)
{
	if (!(f->openmode & O_WRONLY))
		return -EINVAL;
	if (!f->dentry->sb->fs->write)
		return -ENOSYS;
	if (size == 0)
		return 0;
	int count = f->dentry->sb->fs->write(f, f->offset, buf, size);
	if (count > 0)
		f->offset += count;
	return count;
}

int vfs_lseek(file_t *f, u32 offset, int whence)
{
	if (f->dentry->sb->fs->lseek)
		return f->dentry->sb->fs->lseek(f, offset, whence);
	switch (whence) {
	case SEEK_END:
		offset += f->dentry->size - f->offset;
		/* fall through */
	case SEEK_CUR:
		offset += f->offset;
		/* fall through */
	case SEEK_SET:
		if (offset > f->dentry->size)
			return -EINVAL;
		f->offset = offset;
		return 0;
	}
	return -EINVAL;
}

int vfs_dunlink(dentry_t *d)
{
	if (DIR_TYPE(d->mode) == DIR_DIR)
		return -EISDIR;
	if (!d->sb->fs->unlink)
		return -ENOSYS;
	return d->sb->fs->unlink(d);
}

int vfs_unlink(const char *path)
{
	int ret;
	dentry_t *d = lookup_path_common(path, LAST_NO_SLASHES, NULL);
	if (IS_ERR(d))
		return PTR_ERR(d);
	ret = vfs_dunlink(d);
	dentry_put(d);
	return ret;
}

