#include <fs.h>
#include <kernel.h>
#include <list.h>
#include <devfs.h>

#define ROUND_CHUNK(x)	(((x) + 4095) & ~4095)

typedef struct dpriv {
	dentry_t *dentry;
	list_t children;
	list_t header;
	devfs_ops_t *ops;
} dpriv_t;

static dpriv_t *root;

static dentry_t *make_node(dentry_t *parent, const char *name, u32 mode)
{
	dpriv_t *dpriv;
	dpriv_t *parent_dpriv;
	dentry_t *d;
	int err;
       	d = kzalloc(sizeof(dentry_t));
	if (!d) {
		err = -ENOMEM;
		goto out;
	}
	dpriv = kzalloc(sizeof(dpriv_t));
	if (!dpriv) {
		err = -ENOMEM;
		goto fail_dpriv;
	}
	strcpy(d->name, name);
	d->mode = mode;
	d->sb = parent->sb;
	d->priv = dpriv;
	dpriv->dentry = d;
	list_init(&dpriv->children);
	parent_dpriv = (dpriv_t *)parent->priv;
	list_add_tail(&parent_dpriv->children, &dpriv->header);
	return dentry_get(d);
fail_dpriv:
	kfree(d);
out:
	return ERR_PTR(err);
}

static int devfs_mount(superblock_t *sb)
{
	dentry_t *d;
	dpriv_t *dpriv;
       	d = kzalloc(sizeof(dentry_t));
	if (d == NULL)
		goto out;
	sb->root_dentry = dentry_get(d);
	strcpy(d->name, "/");
	d->mode = DIR_DIR;
	d->parent = NULL;
	d->sb = sb;
	dpriv = kzalloc(sizeof(dpriv_t));
	if (dpriv == NULL)
		goto fail_priv;
	dpriv->dentry = d;
	list_init(&dpriv->children);
	list_init(&dpriv->header);
	d->priv = dpriv;
	root = dpriv;

	return 0;
fail_priv:
	kfree(d);
out:
	return -ENOMEM;
}

static dentry_t *devfs_lookup(dentry_t *d, const char *name)
{
	dpriv_t *priv = d->priv;
	list_t *iter;
	if (DIR_TYPE(d->mode) != DIR_DIR)
		return ERR_PTR(-ENOTDIR);
	list_for_each(&priv->children, iter) {
		dpriv_t *i = list_get(iter, dpriv_t, header);
		if (strcmp(i->dentry->name, name) == 0)
			return dentry_get(i->dentry);
	}
	return ERR_PTR(-ENOENT);
}

static int devfs_getdents(dentry_t *d, void *buf, u32 size, int start)
{
	dpriv_t *priv = d->priv;
	list_t *iter;
	int count = 0;
	if (DIR_TYPE(d->mode) != DIR_DIR)
		return -ENOTDIR;
	list_for_each(&priv->children, iter) {
		dpriv_t *i;
		if (start--)
			continue;
		i = list_get(iter, dpriv_t, header);
		if (vfs_add_dentry(&buf, &size, i->dentry->name, i->dentry->mode, i->dentry->size))
			count++;
		else {
			/* if the buffer is insufficient even for the first entry... */
			if (count == 0)
				return -ENOMEM;
			else
				break;
		}
	}
	return count;
}

static int devfs_open(file_t *f, int flags)
{
	devfs_ops_t *ops = ((dpriv_t *)f->dentry->priv)->ops;
	if (((flags & O_RDONLY) && !ops->read) || ((flags & O_WRONLY) && !ops->write)) {
		return -EACCES;
	}
	return ops->open(f, flags);

}

static int devfs_close(file_t *f) {
	return ((dpriv_t *)f->dentry->priv)->ops->close(f);
}

static int devfs_read(file_t *f, u32 offset, char *buf, u32 size) {
	return ((dpriv_t *)f->dentry->priv)->ops->read(f, offset, buf, size);
}

static int devfs_write(file_t *f, u32 offset, const char *buf, u32 size) {
	return ((dpriv_t *)f->dentry->priv)->ops->write(f, offset, buf, size);
}

static fs_t devfs_fs = {
	.name = "devfs",
	.mount = devfs_mount,
	.lookup = devfs_lookup,
	.getdents = devfs_getdents,
	.open = devfs_open,
	.close = devfs_close,
	.read = devfs_read,
	.write = devfs_write,
};

int init_devfs(void)
{
	return register_fs(&devfs_fs);
}

int devfs_register(const char *path, devfs_ops_t *ops) {
	char *name;
	dentry_t *parent = root->dentry;
	while (1) {
		int comp = get_next_component(&path, &name);
		BUG_ON(comp == COMP_NO_MORE);
		if (comp != COMP_NORM) {
			break;
		}
		// create directory if needed
		dentry_t *d = devfs_lookup(parent, name);
		if (IS_ERR(d)) {
			d = make_node(parent, name, DIR_DIR);
		}
		kfree(name);
		if (IS_ERR(d)) {
			return PTR_ERR(d);
		}
		parent = d;
	}
	// last or last with slash
	dentry_t *d = make_node(parent, name, DIR_DIR);
	kfree(name);
	if (IS_ERR(d)) {
		return PTR_ERR(d);
	}
	dentry_t *dev = make_node(d, "dev", DIR_FILE);
	((dpriv_t *)dev->priv)->ops = ops;
	return 0;
}
