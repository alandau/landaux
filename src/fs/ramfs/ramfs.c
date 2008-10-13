#include <fs.h>
#include <kernel.h>
#include <list.h>

typedef struct dpriv {
	dentry_t *dentry;
	list_t children;
	list_t header;
} dpriv_t;

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

static int ramfs_mount(superblock_t *sb)
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

	/* populate ramfs */
	dentry_t *e;
	make_node(d, "a", DIR_FILE);
	make_node(d, "b", DIR_FILE);
	e = make_node(d, "c", DIR_DIR);
	make_node(e, "1", DIR_FILE);

	return 0;
fail_priv:
	kfree(d);
out:
	return -ENOMEM;
}

static dentry_t *ramfs_lookup(dentry_t *d, const char *name)
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

static int ramfs_getdents(dentry_t *d, void *buf, u32 size, int start)
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

static int ramfs_mkdir(dentry_t *d, const char *name)
{
	void *err = make_node(d, name, DIR_DIR);
	if (IS_ERR(err))
		return PTR_ERR(err);
	return 0;
}

static int ramfs_rmdir(dentry_t *d)
{
	dpriv_t *priv = d->priv;
	if (!list_empty(&priv->children))
		return -ENOTEMPTY;
	list_del(&priv->header);
	dentry_put(d);
	return 0;
}
static int ramfs_create(dentry_t *d, const char *name)
{
	void *err = make_node(d, name, DIR_FILE);
	if (IS_ERR(err))
		return PTR_ERR(err);
	return 0;
}

static fs_t ramfs_fs = {
	.name = "ramfs",
	.mount = ramfs_mount,
	.lookup = ramfs_lookup,
	.getdents = ramfs_getdents,
	.mkdir = ramfs_mkdir,
	.rmdir = ramfs_rmdir,
	.create = ramfs_create,
};

int init_ramfs(void)
{
	return register_fs(&ramfs_fs);
}
