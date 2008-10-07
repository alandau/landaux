#include <fs.h>
#include <kernel.h>
#include <list.h>

typedef struct dpriv {
	dentry_t *dentry;
	list_t siblings;
	struct dpriv *child;
} dpriv_t;

static dentry_t *make_node(dentry_t *parent, char *name)
{
	dpriv_t *dpriv;
	dpriv_t *parent_dpriv;
	dentry_t *d = kzalloc(sizeof(dentry_t));
	if (!d)
		goto out;
	dpriv = kzalloc(sizeof(dpriv_t));
	if (!dpriv)
		goto fail_dpriv;
	dentry_get(d);
	strcpy(d->name, name);
	d->sb = parent->sb;
	d->priv = dpriv;
	dpriv->dentry = d;
	list_init(&dpriv->siblings);
	parent_dpriv = (dpriv_t *)parent->priv;
	if (parent_dpriv->child == NULL)
		parent_dpriv->child = dpriv;
	else
		list_add_tail(&parent_dpriv->child->siblings, &dpriv->siblings);
	return d;
fail_dpriv:
	kfree(d);
out:
	return NULL;
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
	d->parent = NULL;
	d->sb = sb;
	dpriv = kzalloc(sizeof(dpriv_t));
	if (dpriv == NULL)
		goto fail_priv;
	dpriv->dentry = d;
	list_init(&dpriv->siblings);
	dpriv->child = NULL;
	d->priv = dpriv;

	/* populate ramfs */
	make_node(d, "a");
	make_node(d, "b");
	make_node(make_node(d, "c"), "1");

	return 0;
fail_priv:
	kfree(d);
out:
	return -ENOMEM;
}

static int ramfs_lookup(superblock_t *sb, dentry_t *d, char *name, dentry_t **new_d)
{
	dpriv_t *priv = d->priv;
	dpriv_t *child = priv->child;
	list_t *iter;
	if (child == NULL)
		return -ENOENT;
	if (strcmp(child->dentry->name, name) == 0) {
		*new_d = dentry_get(child->dentry);
		return 0;
	}
	list_for_each(&child->siblings, iter) {
		dpriv_t *i = list_get(iter, dpriv_t, siblings);
		if (strcmp(i->dentry->name, name) == 0) {
			*new_d = dentry_get(i->dentry);
			return 0;
		}
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
