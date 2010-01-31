/*
 * Copyright (c) 2008 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup fs
 * @{
 */ 

/**
 * @file	fat_ops.c
 * @brief	Implementation of VFS operations for the FAT file system server.
 */

#include "fat.h"
#include "fat_dentry.h"
#include "fat_fat.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <libblock.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/devmap.h>
#include <async.h>
#include <errno.h>
#include <string.h>
#include <byteorder.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <sys/mman.h>
#include <align.h>

#define FAT_NODE(node)	((node) ? (fat_node_t *) (node)->data : NULL)
#define FS_NODE(node)	((node) ? (node)->bp : NULL)

/** Mutex protecting the list of cached free FAT nodes. */
static FIBRIL_MUTEX_INITIALIZE(ffn_mutex);

/** List of cached free FAT nodes. */
static LIST_INITIALIZE(ffn_head);

/*
 * Forward declarations of FAT libfs operations.
 */
static int fat_root_get(fs_node_t **, dev_handle_t);
static int fat_match(fs_node_t **, fs_node_t *, const char *);
static int fat_node_get(fs_node_t **, dev_handle_t, fs_index_t);
static int fat_node_open(fs_node_t *);
static int fat_node_put(fs_node_t *);
static int fat_create_node(fs_node_t **, dev_handle_t, int);
static int fat_destroy_node(fs_node_t *);
static int fat_link(fs_node_t *, fs_node_t *, const char *);
static int fat_unlink(fs_node_t *, fs_node_t *, const char *);
static int fat_has_children(bool *, fs_node_t *);
static fs_index_t fat_index_get(fs_node_t *);
static size_t fat_size_get(fs_node_t *);
static unsigned fat_lnkcnt_get(fs_node_t *);
static char fat_plb_get_char(unsigned);
static bool fat_is_directory(fs_node_t *);
static bool fat_is_file(fs_node_t *node);
static dev_handle_t fat_device_get(fs_node_t *node);

/*
 * Helper functions.
 */
static void fat_node_initialize(fat_node_t *node)
{
	fibril_mutex_initialize(&node->lock);
	node->bp = NULL;
	node->idx = NULL;
	node->type = 0;
	link_initialize(&node->ffn_link);
	node->size = 0;
	node->lnkcnt = 0;
	node->refcnt = 0;
	node->dirty = false;
}

static int fat_node_sync(fat_node_t *node)
{
	block_t *b;
	fat_bs_t *bs;
	fat_dentry_t *d;
	uint16_t bps;
	unsigned dps;
	int rc;
	
	assert(node->dirty);

	bs = block_bb_get(node->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);
	
	/* Read the block that contains the dentry of interest. */
	rc = _fat_block_get(&b, bs, node->idx->dev_handle, node->idx->pfc,
	    (node->idx->pdi * sizeof(fat_dentry_t)) / bps, BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	d = ((fat_dentry_t *)b->data) + (node->idx->pdi % dps);

	d->firstc = host2uint16_t_le(node->firstc);
	if (node->type == FAT_FILE) {
		d->size = host2uint32_t_le(node->size);
	} else if (node->type == FAT_DIRECTORY) {
		d->attr = FAT_ATTR_SUBDIR;
	}
	
	/* TODO: update other fields? (e.g time fields) */
	
	b->dirty = true;		/* need to sync block */
	rc = block_put(b);
	return rc;
}

static int fat_node_fini_by_dev_handle(dev_handle_t dev_handle)
{
	link_t *lnk;
	fat_node_t *nodep;
	int rc;

	/*
	 * We are called from fat_unmounted() and assume that there are already
	 * no nodes belonging to this instance with non-zero refcount. Therefore
	 * it is sufficient to clean up only the FAT free node list.
	 */

restart:
	fibril_mutex_lock(&ffn_mutex);
	for (lnk = ffn_head.next; lnk != &ffn_head; lnk = lnk->next) {
		nodep = list_get_instance(lnk, fat_node_t, ffn_link);
		if (!fibril_mutex_trylock(&nodep->lock)) {
			fibril_mutex_unlock(&ffn_mutex);
			goto restart;
		}
		if (!fibril_mutex_trylock(&nodep->idx->lock)) {
			fibril_mutex_unlock(&nodep->lock);
			fibril_mutex_unlock(&ffn_mutex);
			goto restart;
		}
		if (nodep->idx->dev_handle != dev_handle) {
			fibril_mutex_unlock(&nodep->idx->lock);
			fibril_mutex_unlock(&nodep->lock);
			continue;
		}

		list_remove(&nodep->ffn_link);
		fibril_mutex_unlock(&ffn_mutex);

		/*
		 * We can unlock the node and its index structure because we are
		 * the last player on this playground and VFS is preventing new
		 * players from entering.
		 */
		fibril_mutex_unlock(&nodep->idx->lock);
		fibril_mutex_unlock(&nodep->lock);

		if (nodep->dirty) {
			rc = fat_node_sync(nodep);
			if (rc != EOK)
				return rc;
		}
		nodep->idx->nodep = NULL;
		free(nodep->bp);
		free(nodep);

		/* Need to restart because we changed the ffn_head list. */
		goto restart;
	}
	fibril_mutex_unlock(&ffn_mutex);

	return EOK;
}

static int fat_node_get_new(fat_node_t **nodepp)
{
	fs_node_t *fn;
	fat_node_t *nodep;
	int rc;

	fibril_mutex_lock(&ffn_mutex);
	if (!list_empty(&ffn_head)) {
		/* Try to use a cached free node structure. */
		fat_idx_t *idxp_tmp;
		nodep = list_get_instance(ffn_head.next, fat_node_t, ffn_link);
		if (!fibril_mutex_trylock(&nodep->lock))
			goto skip_cache;
		idxp_tmp = nodep->idx;
		if (!fibril_mutex_trylock(&idxp_tmp->lock)) {
			fibril_mutex_unlock(&nodep->lock);
			goto skip_cache;
		}
		list_remove(&nodep->ffn_link);
		fibril_mutex_unlock(&ffn_mutex);
		if (nodep->dirty) {
			rc = fat_node_sync(nodep);
			if (rc != EOK) {
				idxp_tmp->nodep = NULL;
				fibril_mutex_unlock(&nodep->lock);
				fibril_mutex_unlock(&idxp_tmp->lock);
				free(nodep->bp);
				free(nodep);
				return rc;
			}
		}
		idxp_tmp->nodep = NULL;
		fibril_mutex_unlock(&nodep->lock);
		fibril_mutex_unlock(&idxp_tmp->lock);
		fn = FS_NODE(nodep);
	} else {
skip_cache:
		/* Try to allocate a new node structure. */
		fibril_mutex_unlock(&ffn_mutex);
		fn = (fs_node_t *)malloc(sizeof(fs_node_t));
		if (!fn)
			return ENOMEM;
		nodep = (fat_node_t *)malloc(sizeof(fat_node_t));
		if (!nodep) {
			free(fn);
			return ENOMEM;
		}
	}
	fat_node_initialize(nodep);
	fs_node_initialize(fn);
	fn->data = nodep;
	nodep->bp = fn;
	
	*nodepp = nodep;
	return EOK;
}

/** Internal version of fat_node_get().
 *
 * @param idxp		Locked index structure.
 */
static int fat_node_get_core(fat_node_t **nodepp, fat_idx_t *idxp)
{
	block_t *b;
	fat_bs_t *bs;
	fat_dentry_t *d;
	fat_node_t *nodep = NULL;
	unsigned bps;
	unsigned spc;
	unsigned dps;
	int rc;

	if (idxp->nodep) {
		/*
		 * We are lucky.
		 * The node is already instantiated in memory.
		 */
		fibril_mutex_lock(&idxp->nodep->lock);
		if (!idxp->nodep->refcnt++) {
			fibril_mutex_lock(&ffn_mutex);
			list_remove(&idxp->nodep->ffn_link);
			fibril_mutex_unlock(&ffn_mutex);
		}
		fibril_mutex_unlock(&idxp->nodep->lock);
		*nodepp = idxp->nodep;
		return EOK;
	}

	/*
	 * We must instantiate the node from the file system.
	 */
	
	assert(idxp->pfc);

	rc = fat_node_get_new(&nodep);
	if (rc != EOK)
		return rc;

	bs = block_bb_get(idxp->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	spc = bs->spc;
	dps = bps / sizeof(fat_dentry_t);

	/* Read the block that contains the dentry of interest. */
	rc = _fat_block_get(&b, bs, idxp->dev_handle, idxp->pfc,
	    (idxp->pdi * sizeof(fat_dentry_t)) / bps, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		(void) fat_node_put(FS_NODE(nodep));
		return rc;
	}

	d = ((fat_dentry_t *)b->data) + (idxp->pdi % dps);
	if (d->attr & FAT_ATTR_SUBDIR) {
		/* 
		 * The only directory which does not have this bit set is the
		 * root directory itself. The root directory node is handled
		 * and initialized elsewhere.
		 */
		nodep->type = FAT_DIRECTORY;
		/*
		 * Unfortunately, the 'size' field of the FAT dentry is not
		 * defined for the directory entry type. We must determine the
		 * size of the directory by walking the FAT.
		 */
		uint16_t clusters;
		rc = fat_clusters_get(&clusters, bs, idxp->dev_handle,
		    uint16_t_le2host(d->firstc));
		if (rc != EOK) {
			(void) fat_node_put(FS_NODE(nodep));
			return rc;
		}
		nodep->size = bps * spc * clusters;
	} else {
		nodep->type = FAT_FILE;
		nodep->size = uint32_t_le2host(d->size);
	}
	nodep->firstc = uint16_t_le2host(d->firstc);
	nodep->lnkcnt = 1;
	nodep->refcnt = 1;

	rc = block_put(b);
	if (rc != EOK) {
		(void) fat_node_put(FS_NODE(nodep));
		return rc;
	}

	/* Link the idx structure with the node structure. */
	nodep->idx = idxp;
	idxp->nodep = nodep;

	*nodepp = nodep;
	return EOK;
}

/*
 * FAT libfs operations.
 */

int fat_root_get(fs_node_t **rfn, dev_handle_t dev_handle)
{
	return fat_node_get(rfn, dev_handle, 0);
}

int fat_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	fat_bs_t *bs;
	fat_node_t *parentp = FAT_NODE(pfn);
	char name[FAT_NAME_LEN + 1 + FAT_EXT_LEN + 1];
	unsigned i, j;
	unsigned bps;		/* bytes per sector */
	unsigned dps;		/* dentries per sector */
	unsigned blocks;
	fat_dentry_t *d;
	block_t *b;
	int rc;

	fibril_mutex_lock(&parentp->idx->lock);
	bs = block_bb_get(parentp->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);
	blocks = parentp->size / bps;
	for (i = 0; i < blocks; i++) {
		rc = fat_block_get(&b, bs, parentp, i, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			fibril_mutex_unlock(&parentp->idx->lock);
			return rc;
		}
		for (j = 0; j < dps; j++) { 
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_FREE:
				continue;
			case FAT_DENTRY_LAST:
				/* miss */
				rc = block_put(b);
				fibril_mutex_unlock(&parentp->idx->lock);
				*rfn = NULL;
				return rc;
			default:
			case FAT_DENTRY_VALID:
				fat_dentry_name_get(d, name);
				break;
			}
			if (fat_dentry_namecmp(name, component) == 0) {
				/* hit */
				fat_node_t *nodep;
				/*
				 * Assume tree hierarchy for locking.  We
				 * already have the parent and now we are going
				 * to lock the child.  Never lock in the oposite
				 * order.
				 */
				fat_idx_t *idx = fat_idx_get_by_pos(
				    parentp->idx->dev_handle, parentp->firstc,
				    i * dps + j);
				fibril_mutex_unlock(&parentp->idx->lock);
				if (!idx) {
					/*
					 * Can happen if memory is low or if we
					 * run out of 32-bit indices.
					 */
					rc = block_put(b);
					return (rc == EOK) ? ENOMEM : rc;
				}
				rc = fat_node_get_core(&nodep, idx);
				fibril_mutex_unlock(&idx->lock);
				if (rc != EOK) {
					(void) block_put(b);
					return rc;
				}
				*rfn = FS_NODE(nodep);
				rc = block_put(b);
				if (rc != EOK)
					(void) fat_node_put(*rfn);
				return rc;
			}
		}
		rc = block_put(b);
		if (rc != EOK) {
			fibril_mutex_unlock(&parentp->idx->lock);
			return rc;
		}
	}

	fibril_mutex_unlock(&parentp->idx->lock);
	*rfn = NULL;
	return EOK;
}

/** Instantiate a FAT in-core node. */
int fat_node_get(fs_node_t **rfn, dev_handle_t dev_handle, fs_index_t index)
{
	fat_node_t *nodep;
	fat_idx_t *idxp;
	int rc;

	idxp = fat_idx_get_by_index(dev_handle, index);
	if (!idxp) {
		*rfn = NULL;
		return EOK;
	}
	/* idxp->lock held */
	rc = fat_node_get_core(&nodep, idxp);
	fibril_mutex_unlock(&idxp->lock);
	if (rc == EOK)
		*rfn = FS_NODE(nodep);
	return rc;
}

int fat_node_open(fs_node_t *fn)
{
	/*
	 * Opening a file is stateless, nothing
	 * to be done here.
	 */
	return EOK;
}

int fat_node_put(fs_node_t *fn)
{
	fat_node_t *nodep = FAT_NODE(fn);
	bool destroy = false;

	fibril_mutex_lock(&nodep->lock);
	if (!--nodep->refcnt) {
		if (nodep->idx) {
			fibril_mutex_lock(&ffn_mutex);
			list_append(&nodep->ffn_link, &ffn_head);
			fibril_mutex_unlock(&ffn_mutex);
		} else {
			/*
			 * The node does not have any index structure associated
			 * with itself. This can only mean that we are releasing
			 * the node after a failed attempt to allocate the index
			 * structure for it.
			 */
			destroy = true;
		}
	}
	fibril_mutex_unlock(&nodep->lock);
	if (destroy) {
		free(nodep->bp);
		free(nodep);
	}
	return EOK;
}

int fat_create_node(fs_node_t **rfn, dev_handle_t dev_handle, int flags)
{
	fat_idx_t *idxp;
	fat_node_t *nodep;
	fat_bs_t *bs;
	fat_cluster_t mcl, lcl;
	uint16_t bps;
	int rc;

	bs = block_bb_get(dev_handle);
	bps = uint16_t_le2host(bs->bps);
	if (flags & L_DIRECTORY) {
		/* allocate a cluster */
		rc = fat_alloc_clusters(bs, dev_handle, 1, &mcl, &lcl);
		if (rc != EOK)
			return rc;
		/* populate the new cluster with unused dentries */
		rc = fat_zero_cluster(bs, dev_handle, mcl);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, dev_handle, mcl);
			return rc;
		}
	}

	rc = fat_node_get_new(&nodep);
	if (rc != EOK) {
		(void) fat_free_clusters(bs, dev_handle, mcl);
		return rc;
	}
	rc = fat_idx_get_new(&idxp, dev_handle);
	if (rc != EOK) {
		(void) fat_free_clusters(bs, dev_handle, mcl);	
		(void) fat_node_put(FS_NODE(nodep));
		return rc;
	}
	/* idxp->lock held */
	if (flags & L_DIRECTORY) {
		nodep->type = FAT_DIRECTORY;
		nodep->firstc = mcl;
		nodep->size = bps * bs->spc;
	} else {
		nodep->type = FAT_FILE;
		nodep->firstc = FAT_CLST_RES0;
		nodep->size = 0;
	}
	nodep->lnkcnt = 0;	/* not linked anywhere */
	nodep->refcnt = 1;
	nodep->dirty = true;

	nodep->idx = idxp;
	idxp->nodep = nodep;

	fibril_mutex_unlock(&idxp->lock);
	*rfn = FS_NODE(nodep);
	return EOK;
}

int fat_destroy_node(fs_node_t *fn)
{
	fat_node_t *nodep = FAT_NODE(fn);
	fat_bs_t *bs;
	bool has_children;
	int rc;

	/*
	 * The node is not reachable from the file system. This means that the
	 * link count should be zero and that the index structure cannot be
	 * found in the position hash. Obviously, we don't need to lock the node
	 * nor its index structure.
	 */
	assert(nodep->lnkcnt == 0);

	/*
	 * The node may not have any children.
	 */
	rc = fat_has_children(&has_children, fn);
	if (rc != EOK)
		return rc;
	assert(!has_children);

	bs = block_bb_get(nodep->idx->dev_handle);
	if (nodep->firstc != FAT_CLST_RES0) {
		assert(nodep->size);
		/* Free all clusters allocated to the node. */
		rc = fat_free_clusters(bs, nodep->idx->dev_handle,
		    nodep->firstc);
	}

	fat_idx_destroy(nodep->idx);
	free(nodep->bp);
	free(nodep);
	return rc;
}

int fat_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	fat_node_t *parentp = FAT_NODE(pfn);
	fat_node_t *childp = FAT_NODE(cfn);
	fat_dentry_t *d;
	fat_bs_t *bs;
	block_t *b;
	unsigned i, j;
	uint16_t bps;
	unsigned dps;
	unsigned blocks;
	fat_cluster_t mcl, lcl;
	int rc;

	fibril_mutex_lock(&childp->lock);
	if (childp->lnkcnt == 1) {
		/*
		 * On FAT, we don't support multiple hard links.
		 */
		fibril_mutex_unlock(&childp->lock);
		return EMLINK;
	}
	assert(childp->lnkcnt == 0);
	fibril_mutex_unlock(&childp->lock);

	if (!fat_dentry_name_verify(name)) {
		/*
		 * Attempt to create unsupported name.
		 */
		return ENOTSUP;
	}

	/*
	 * Get us an unused parent node's dentry or grow the parent and allocate
	 * a new one.
	 */
	
	fibril_mutex_lock(&parentp->idx->lock);
	bs = block_bb_get(parentp->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);

	blocks = parentp->size / bps;

	for (i = 0; i < blocks; i++) {
		rc = fat_block_get(&b, bs, parentp, i, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			fibril_mutex_unlock(&parentp->idx->lock);
			return rc;
		}
		for (j = 0; j < dps; j++) {
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_VALID:
				/* skipping used and meta entries */
				continue;
			case FAT_DENTRY_FREE:
			case FAT_DENTRY_LAST:
				/* found an empty slot */
				goto hit;
			}
		}
		rc = block_put(b);
		if (rc != EOK) {
			fibril_mutex_unlock(&parentp->idx->lock);
			return rc;
		}
	}
	j = 0;
	
	/*
	 * We need to grow the parent in order to create a new unused dentry.
	 */
	if (parentp->firstc == FAT_CLST_ROOT) {
		/* Can't grow the root directory. */
		fibril_mutex_unlock(&parentp->idx->lock);
		return ENOSPC;
	}
	rc = fat_alloc_clusters(bs, parentp->idx->dev_handle, 1, &mcl, &lcl);
	if (rc != EOK) {
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}
	rc = fat_zero_cluster(bs, parentp->idx->dev_handle, mcl);
	if (rc != EOK) {
		(void) fat_free_clusters(bs, parentp->idx->dev_handle, mcl);
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}
	rc = fat_append_clusters(bs, parentp, mcl);
	if (rc != EOK) {
		(void) fat_free_clusters(bs, parentp->idx->dev_handle, mcl);
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}
	parentp->size += bps * bs->spc;
	parentp->dirty = true;		/* need to sync node */
	rc = fat_block_get(&b, bs, parentp, i, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}
	d = (fat_dentry_t *)b->data;

hit:
	/*
	 * At this point we only establish the link between the parent and the
	 * child.  The dentry, except of the name and the extension, will remain
	 * uninitialized until the corresponding node is synced. Thus the valid
	 * dentry data is kept in the child node structure.
	 */
	memset(d, 0, sizeof(fat_dentry_t));
	fat_dentry_name_set(d, name);
	b->dirty = true;		/* need to sync block */
	rc = block_put(b);
	fibril_mutex_unlock(&parentp->idx->lock);
	if (rc != EOK) 
		return rc;

	fibril_mutex_lock(&childp->idx->lock);
	
	/*
	 * If possible, create the Sub-directory Identifier Entry and the
	 * Sub-directory Parent Pointer Entry (i.e. "." and ".."). These entries
	 * are not mandatory according to Standard ECMA-107 and HelenOS VFS does
	 * not use them anyway, so this is rather a sign of our good will.
	 */
	rc = fat_block_get(&b, bs, childp, 0, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		/*
		 * Rather than returning an error, simply skip the creation of
		 * these two entries.
		 */
		goto skip_dots;
	}
	d = (fat_dentry_t *)b->data;
	if (fat_classify_dentry(d) == FAT_DENTRY_LAST ||
	    str_cmp(d->name, FAT_NAME_DOT) == 0) {
	   	memset(d, 0, sizeof(fat_dentry_t));
	   	str_cpy(d->name, 8, FAT_NAME_DOT);
		str_cpy(d->ext, 3, FAT_EXT_PAD);
		d->attr = FAT_ATTR_SUBDIR;
		d->firstc = host2uint16_t_le(childp->firstc);
		/* TODO: initialize also the date/time members. */
	}
	d++;
	if (fat_classify_dentry(d) == FAT_DENTRY_LAST ||
	    str_cmp(d->name, FAT_NAME_DOT_DOT) == 0) {
		memset(d, 0, sizeof(fat_dentry_t));
		str_cpy(d->name, 8, FAT_NAME_DOT_DOT);
		str_cpy(d->ext, 3, FAT_EXT_PAD);
		d->attr = FAT_ATTR_SUBDIR;
		d->firstc = (parentp->firstc == FAT_CLST_ROOT) ?
		    host2uint16_t_le(FAT_CLST_RES0) :
		    host2uint16_t_le(parentp->firstc);
		/* TODO: initialize also the date/time members. */
	}
	b->dirty = true;		/* need to sync block */
	/*
	 * Ignore the return value as we would have fallen through on error
	 * anyway.
	 */
	(void) block_put(b);
skip_dots:

	childp->idx->pfc = parentp->firstc;
	childp->idx->pdi = i * dps + j;
	fibril_mutex_unlock(&childp->idx->lock);

	fibril_mutex_lock(&childp->lock);
	childp->lnkcnt = 1;
	childp->dirty = true;		/* need to sync node */
	fibril_mutex_unlock(&childp->lock);

	/*
	 * Hash in the index structure into the position hash.
	 */
	fat_idx_hashin(childp->idx);

	return EOK;
}

int fat_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	fat_node_t *parentp = FAT_NODE(pfn);
	fat_node_t *childp = FAT_NODE(cfn);
	fat_bs_t *bs;
	fat_dentry_t *d;
	uint16_t bps;
	block_t *b;
	bool has_children;
	int rc;

	if (!parentp)
		return EBUSY;
	
	rc = fat_has_children(&has_children, cfn);
	if (rc != EOK)
		return rc;
	if (has_children)
		return ENOTEMPTY;

	fibril_mutex_lock(&parentp->lock);
	fibril_mutex_lock(&childp->lock);
	assert(childp->lnkcnt == 1);
	fibril_mutex_lock(&childp->idx->lock);
	bs = block_bb_get(childp->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);

	rc = _fat_block_get(&b, bs, childp->idx->dev_handle, childp->idx->pfc,
	    (childp->idx->pdi * sizeof(fat_dentry_t)) / bps,
	    BLOCK_FLAGS_NONE);
	if (rc != EOK) 
		goto error;
	d = (fat_dentry_t *)b->data +
	    (childp->idx->pdi % (bps / sizeof(fat_dentry_t)));
	/* mark the dentry as not-currently-used */
	d->name[0] = FAT_DENTRY_ERASED;
	b->dirty = true;		/* need to sync block */
	rc = block_put(b);
	if (rc != EOK)
		goto error;

	/* remove the index structure from the position hash */
	fat_idx_hashout(childp->idx);
	/* clear position information */
	childp->idx->pfc = FAT_CLST_RES0;
	childp->idx->pdi = 0;
	fibril_mutex_unlock(&childp->idx->lock);
	childp->lnkcnt = 0;
	childp->dirty = true;
	fibril_mutex_unlock(&childp->lock);
	fibril_mutex_unlock(&parentp->lock);

	return EOK;

error:
	fibril_mutex_unlock(&parentp->idx->lock);
	fibril_mutex_unlock(&childp->lock);
	fibril_mutex_unlock(&childp->idx->lock);
	return rc;
}

int fat_has_children(bool *has_children, fs_node_t *fn)
{
	fat_bs_t *bs;
	fat_node_t *nodep = FAT_NODE(fn);
	unsigned bps;
	unsigned dps;
	unsigned blocks;
	block_t *b;
	unsigned i, j;
	int rc;

	if (nodep->type != FAT_DIRECTORY) {
		*has_children = false;
		return EOK;
	}
	
	fibril_mutex_lock(&nodep->idx->lock);
	bs = block_bb_get(nodep->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);

	blocks = nodep->size / bps;

	for (i = 0; i < blocks; i++) {
		fat_dentry_t *d;
	
		rc = fat_block_get(&b, bs, nodep, i, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			fibril_mutex_unlock(&nodep->idx->lock);
			return rc;
		}
		for (j = 0; j < dps; j++) {
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_FREE:
				continue;
			case FAT_DENTRY_LAST:
				rc = block_put(b);
				fibril_mutex_unlock(&nodep->idx->lock);
				*has_children = false;
				return rc;
			default:
			case FAT_DENTRY_VALID:
				rc = block_put(b);
				fibril_mutex_unlock(&nodep->idx->lock);
				*has_children = true;
				return rc;
			}
		}
		rc = block_put(b);
		if (rc != EOK) {
			fibril_mutex_unlock(&nodep->idx->lock);
			return rc;	
		}
	}

	fibril_mutex_unlock(&nodep->idx->lock);
	*has_children = false;
	return EOK;
}


fs_index_t fat_index_get(fs_node_t *fn)
{
	return FAT_NODE(fn)->idx->index;
}

size_t fat_size_get(fs_node_t *fn)
{
	return FAT_NODE(fn)->size;
}

unsigned fat_lnkcnt_get(fs_node_t *fn)
{
	return FAT_NODE(fn)->lnkcnt;
}

char fat_plb_get_char(unsigned pos)
{
	return fat_reg.plb_ro[pos % PLB_SIZE];
}

bool fat_is_directory(fs_node_t *fn)
{
	return FAT_NODE(fn)->type == FAT_DIRECTORY;
}

bool fat_is_file(fs_node_t *fn)
{
	return FAT_NODE(fn)->type == FAT_FILE;
}

dev_handle_t fat_device_get(fs_node_t *node)
{
	return 0;
}

/** libfs operations */
libfs_ops_t fat_libfs_ops = {
	.root_get = fat_root_get,
	.match = fat_match,
	.node_get = fat_node_get,
	.node_open = fat_node_open,
	.node_put = fat_node_put,
	.create = fat_create_node,
	.destroy = fat_destroy_node,
	.link = fat_link,
	.unlink = fat_unlink,
	.has_children = fat_has_children,
	.index_get = fat_index_get,
	.size_get = fat_size_get,
	.lnkcnt_get = fat_lnkcnt_get,
	.plb_get_char = fat_plb_get_char,
	.is_directory = fat_is_directory,
	.is_file = fat_is_file,
	.device_get = fat_device_get
};

/*
 * VFS operations.
 */

void fat_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	enum cache_mode cmode;
	fat_bs_t *bs;
	uint16_t bps;
	uint16_t rde;
	int rc;

	/* accept the mount options */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}
	char *opts = malloc(size + 1);
	if (!opts) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	ipcarg_t retval = async_data_write_finalize(callid, opts, size);
	if (retval != EOK) {
		ipc_answer_0(rid, retval);
		free(opts);
		return;
	}
	opts[size] = '\0';

	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;

	free(opts);

	/* initialize libblock */
	rc = block_init(dev_handle, BS_SIZE);
	if (rc != EOK) {
		ipc_answer_0(rid, rc);
		return;
	}

	/* prepare the boot block */
	rc = block_bb_read(dev_handle, BS_BLOCK);
	if (rc != EOK) {
		block_fini(dev_handle);
		ipc_answer_0(rid, rc);
		return;
	}

	/* get the buffer with the boot sector */
	bs = block_bb_get(dev_handle);
	
	/* Read the number of root directory entries. */
	bps = uint16_t_le2host(bs->bps);
	rde = uint16_t_le2host(bs->root_ent_max);

	if (bps != BS_SIZE) {
		block_fini(dev_handle);
		ipc_answer_0(rid, ENOTSUP);
		return;
	}

	/* Initialize the block cache */
	rc = block_cache_init(dev_handle, bps, 0 /* XXX */, cmode);
	if (rc != EOK) {
		block_fini(dev_handle);
		ipc_answer_0(rid, rc);
		return;
	}

	/* Do some simple sanity checks on the file system. */
	rc = fat_sanity_check(bs, dev_handle);
	if (rc != EOK) {
		(void) block_cache_fini(dev_handle);
		block_fini(dev_handle);
		ipc_answer_0(rid, rc);
		return;
	}

	rc = fat_idx_init_by_dev_handle(dev_handle);
	if (rc != EOK) {
		(void) block_cache_fini(dev_handle);
		block_fini(dev_handle);
		ipc_answer_0(rid, rc);
		return;
	}

	/* Initialize the root node. */
	fs_node_t *rfn = (fs_node_t *)malloc(sizeof(fs_node_t));
	if (!rfn) {
		(void) block_cache_fini(dev_handle);
		block_fini(dev_handle);
		fat_idx_fini_by_dev_handle(dev_handle);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	fs_node_initialize(rfn);
	fat_node_t *rootp = (fat_node_t *)malloc(sizeof(fat_node_t));
	if (!rootp) {
		free(rfn);
		(void) block_cache_fini(dev_handle);
		block_fini(dev_handle);
		fat_idx_fini_by_dev_handle(dev_handle);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	fat_node_initialize(rootp);

	fat_idx_t *ridxp = fat_idx_get_by_pos(dev_handle, FAT_CLST_ROOTPAR, 0);
	if (!ridxp) {
		free(rfn);
		free(rootp);
		(void) block_cache_fini(dev_handle);
		block_fini(dev_handle);
		fat_idx_fini_by_dev_handle(dev_handle);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	assert(ridxp->index == 0);
	/* ridxp->lock held */

	rootp->type = FAT_DIRECTORY;
	rootp->firstc = FAT_CLST_ROOT;
	rootp->refcnt = 1;
	rootp->lnkcnt = 0;	/* FS root is not linked */
	rootp->size = rde * sizeof(fat_dentry_t);
	rootp->idx = ridxp;
	ridxp->nodep = rootp;
	rootp->bp = rfn;
	rfn->data = rootp;
	
	fibril_mutex_unlock(&ridxp->lock);

	ipc_answer_3(rid, EOK, ridxp->index, rootp->size, rootp->lnkcnt);
}

void fat_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&fat_libfs_ops, fat_reg.fs_handle, rid, request);
}

void fat_unmounted(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	fs_node_t *fn;
	fat_node_t *nodep;
	int rc;

	rc = fat_root_get(&fn, dev_handle);
	if (rc != EOK) {
		ipc_answer_0(rid, rc);
		return;
	}
	nodep = FAT_NODE(fn);

	/*
	 * We expect exactly two references on the root node. One for the
	 * fat_root_get() above and one created in fat_mounted().
	 */
	if (nodep->refcnt != 2) {
		(void) fat_node_put(fn);
		ipc_answer_0(rid, EBUSY);
		return;
	}
	
	/*
	 * Put the root node and force it to the FAT free node list.
	 */
	(void) fat_node_put(fn);
	(void) fat_node_put(fn);

	/*
	 * Perform cleanup of the node structures, index structures and
	 * associated data. Write back this file system's dirty blocks and
	 * stop using libblock for this instance.
	 */
	(void) fat_node_fini_by_dev_handle(dev_handle);
	fat_idx_fini_by_dev_handle(dev_handle);
	(void) block_cache_fini(dev_handle);
	block_fini(dev_handle);

	ipc_answer_0(rid, EOK);
}

void fat_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_unmount(&fat_libfs_ops, rid, request);
}

void fat_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&fat_libfs_ops, fat_reg.fs_handle, rid, request);
}

void fat_read(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	off_t pos = (off_t)IPC_GET_ARG3(*request);
	fs_node_t *fn;
	fat_node_t *nodep;
	fat_bs_t *bs;
	uint16_t bps;
	size_t bytes;
	block_t *b;
	int rc;

	rc = fat_node_get(&fn, dev_handle, index);
	if (rc != EOK) {
		ipc_answer_0(rid, rc);
		return;
	}
	if (!fn) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	nodep = FAT_NODE(fn);

	ipc_callid_t callid;
	size_t len;
	if (!async_data_read_receive(&callid, &len)) {
		fat_node_put(fn);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	bs = block_bb_get(dev_handle);
	bps = uint16_t_le2host(bs->bps);

	if (nodep->type == FAT_FILE) {
		/*
		 * Our strategy for regular file reads is to read one block at
		 * most and make use of the possibility to return less data than
		 * requested. This keeps the code very simple.
		 */
		if (pos >= nodep->size) {
			/* reading beyond the EOF */
			bytes = 0;
			(void) async_data_read_finalize(callid, NULL, 0);
		} else {
			bytes = min(len, bps - pos % bps);
			bytes = min(bytes, nodep->size - pos);
			rc = fat_block_get(&b, bs, nodep, pos / bps,
			    BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				fat_node_put(fn);
				ipc_answer_0(callid, rc);
				ipc_answer_0(rid, rc);
				return;
			}
			(void) async_data_read_finalize(callid, b->data + pos % bps,
			    bytes);
			rc = block_put(b);
			if (rc != EOK) {
				fat_node_put(fn);
				ipc_answer_0(rid, rc);
				return;
			}
		}
	} else {
		unsigned bnum;
		off_t spos = pos;
		char name[FAT_NAME_LEN + 1 + FAT_EXT_LEN + 1];
		fat_dentry_t *d;

		assert(nodep->type == FAT_DIRECTORY);
		assert(nodep->size % bps == 0);
		assert(bps % sizeof(fat_dentry_t) == 0);

		/*
		 * Our strategy for readdir() is to use the position pointer as
		 * an index into the array of all dentries. On entry, it points
		 * to the first unread dentry. If we skip any dentries, we bump
		 * the position pointer accordingly.
		 */
		bnum = (pos * sizeof(fat_dentry_t)) / bps;
		while (bnum < nodep->size / bps) {
			off_t o;

			rc = fat_block_get(&b, bs, nodep, bnum,
			    BLOCK_FLAGS_NONE);
			if (rc != EOK)
				goto err;
			for (o = pos % (bps / sizeof(fat_dentry_t));
			    o < bps / sizeof(fat_dentry_t);
			    o++, pos++) {
				d = ((fat_dentry_t *)b->data) + o;
				switch (fat_classify_dentry(d)) {
				case FAT_DENTRY_SKIP:
				case FAT_DENTRY_FREE:
					continue;
				case FAT_DENTRY_LAST:
					rc = block_put(b);
					if (rc != EOK)
						goto err;
					goto miss;
				default:
				case FAT_DENTRY_VALID:
					fat_dentry_name_get(d, name);
					rc = block_put(b);
					if (rc != EOK)
						goto err;
					goto hit;
				}
			}
			rc = block_put(b);
			if (rc != EOK)
				goto err;
			bnum++;
		}
miss:
		rc = fat_node_put(fn);
		ipc_answer_0(callid, rc != EOK ? rc : ENOENT);
		ipc_answer_1(rid, rc != EOK ? rc : ENOENT, 0);
		return;

err:
		(void) fat_node_put(fn);
		ipc_answer_0(callid, rc);
		ipc_answer_0(rid, rc);
		return;

hit:
		(void) async_data_read_finalize(callid, name, str_size(name) + 1);
		bytes = (pos - spos) + 1;
	}

	rc = fat_node_put(fn);
	ipc_answer_1(rid, rc, (ipcarg_t)bytes);
}

void fat_write(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	off_t pos = (off_t)IPC_GET_ARG3(*request);
	fs_node_t *fn;
	fat_node_t *nodep;
	fat_bs_t *bs;
	size_t bytes, size;
	block_t *b;
	uint16_t bps;
	unsigned spc;
	unsigned bpc;		/* bytes per cluster */
	off_t boundary;
	int flags = BLOCK_FLAGS_NONE;
	int rc;
	
	rc = fat_node_get(&fn, dev_handle, index);
	if (rc != EOK) {
		ipc_answer_0(rid, rc);
		return;
	}
	if (!fn) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	nodep = FAT_NODE(fn);
	
	ipc_callid_t callid;
	size_t len;
	if (!async_data_write_receive(&callid, &len)) {
		(void) fat_node_put(fn);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(rid, EINVAL);
		return;
	}

	bs = block_bb_get(dev_handle);
	bps = uint16_t_le2host(bs->bps);
	spc = bs->spc;
	bpc = bps * spc;

	/*
	 * In all scenarios, we will attempt to write out only one block worth
	 * of data at maximum. There might be some more efficient approaches,
	 * but this one greatly simplifies fat_write(). Note that we can afford
	 * to do this because the client must be ready to handle the return
	 * value signalizing a smaller number of bytes written. 
	 */ 
	bytes = min(len, bps - pos % bps);
	if (bytes == bps)
		flags |= BLOCK_FLAGS_NOREAD;
	
	boundary = ROUND_UP(nodep->size, bpc);
	if (pos < boundary) {
		/*
		 * This is the easier case - we are either overwriting already
		 * existing contents or writing behind the EOF, but still within
		 * the limits of the last cluster. The node size may grow to the
		 * next block size boundary.
		 */
		rc = fat_fill_gap(bs, nodep, FAT_CLST_RES0, pos);
		if (rc != EOK) {
			(void) fat_node_put(fn);
			ipc_answer_0(callid, rc);
			ipc_answer_0(rid, rc);
			return;
		}
		rc = fat_block_get(&b, bs, nodep, pos / bps, flags);
		if (rc != EOK) {
			(void) fat_node_put(fn);
			ipc_answer_0(callid, rc);
			ipc_answer_0(rid, rc);
			return;
		}
		(void) async_data_write_finalize(callid, b->data + pos % bps,
		    bytes);
		b->dirty = true;		/* need to sync block */
		rc = block_put(b);
		if (rc != EOK) {
			(void) fat_node_put(fn);
			ipc_answer_0(rid, rc);
			return;
		}
		if (pos + bytes > nodep->size) {
			nodep->size = pos + bytes;
			nodep->dirty = true;	/* need to sync node */
		}
		size = nodep->size;
		rc = fat_node_put(fn);
		ipc_answer_2(rid, rc, bytes, nodep->size);
		return;
	} else {
		/*
		 * This is the more difficult case. We must allocate new
		 * clusters for the node and zero them out.
		 */
		unsigned nclsts;
		fat_cluster_t mcl, lcl; 
 
		nclsts = (ROUND_UP(pos + bytes, bpc) - boundary) / bpc;
		/* create an independent chain of nclsts clusters in all FATs */
		rc = fat_alloc_clusters(bs, dev_handle, nclsts, &mcl, &lcl);
		if (rc != EOK) {
			/* could not allocate a chain of nclsts clusters */
			(void) fat_node_put(fn);
			ipc_answer_0(callid, rc);
			ipc_answer_0(rid, rc);
			return;
		}
		/* zero fill any gaps */
		rc = fat_fill_gap(bs, nodep, mcl, pos);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, dev_handle, mcl);
			(void) fat_node_put(fn);
			ipc_answer_0(callid, rc);
			ipc_answer_0(rid, rc);
			return;
		}
		rc = _fat_block_get(&b, bs, dev_handle, lcl, (pos / bps) % spc,
		    flags);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, dev_handle, mcl);
			(void) fat_node_put(fn);
			ipc_answer_0(callid, rc);
			ipc_answer_0(rid, rc);
			return;
		}
		(void) async_data_write_finalize(callid, b->data + pos % bps,
		    bytes);
		b->dirty = true;		/* need to sync block */
		rc = block_put(b);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, dev_handle, mcl);
			(void) fat_node_put(fn);
			ipc_answer_0(rid, rc);
			return;
		}
		/*
		 * Append the cluster chain starting in mcl to the end of the
		 * node's cluster chain.
		 */
		rc = fat_append_clusters(bs, nodep, mcl);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, dev_handle, mcl);
			(void) fat_node_put(fn);
			ipc_answer_0(rid, rc);
			return;
		}
		nodep->size = size = pos + bytes;
		nodep->dirty = true;		/* need to sync node */
		rc = fat_node_put(fn);
		ipc_answer_2(rid, rc, bytes, size);
		return;
	}
}

void fat_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	size_t size = (off_t)IPC_GET_ARG3(*request);
	fs_node_t *fn;
	fat_node_t *nodep;
	fat_bs_t *bs;
	uint16_t bps;
	uint8_t spc;
	unsigned bpc;	/* bytes per cluster */
	int rc;

	rc = fat_node_get(&fn, dev_handle, index);
	if (rc != EOK) {
		ipc_answer_0(rid, rc);
		return;
	}
	if (!fn) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	nodep = FAT_NODE(fn);

	bs = block_bb_get(dev_handle);
	bps = uint16_t_le2host(bs->bps);
	spc = bs->spc;
	bpc = bps * spc;

	if (nodep->size == size) {
		rc = EOK;
	} else if (nodep->size < size) {
		/*
		 * The standard says we have the freedom to grow the node.
		 * For now, we simply return an error.
		 */
		rc = EINVAL;
	} else if (ROUND_UP(nodep->size, bpc) == ROUND_UP(size, bpc)) {
		/*
		 * The node will be shrunk, but no clusters will be deallocated.
		 */
		nodep->size = size;
		nodep->dirty = true;		/* need to sync node */
		rc = EOK;	
	} else {
		/*
		 * The node will be shrunk, clusters will be deallocated.
		 */
		if (size == 0) {
			rc = fat_chop_clusters(bs, nodep, FAT_CLST_RES0);
			if (rc != EOK)
				goto out;
		} else {
			fat_cluster_t lastc;
			rc = fat_cluster_walk(bs, dev_handle, nodep->firstc,
			    &lastc, NULL, (size - 1) / bpc);
			if (rc != EOK)
				goto out;
			rc = fat_chop_clusters(bs, nodep, lastc);
			if (rc != EOK)
				goto out;
		}
		nodep->size = size;
		nodep->dirty = true;		/* need to sync node */
		rc = EOK;	
	}
out:
	fat_node_put(fn);
	ipc_answer_0(rid, rc);
	return;
}

void fat_close(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, EOK);
}

void fat_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	fs_node_t *fn;
	int rc;

	rc = fat_node_get(&fn, dev_handle, index);
	if (rc != EOK) {
		ipc_answer_0(rid, rc);
		return;
	}
	if (!fn) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	rc = fat_destroy_node(fn);
	ipc_answer_0(rid, rc);
}

void fat_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_open_node(&fat_libfs_ops, fat_reg.fs_handle, rid, request);
}

void fat_stat(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_stat(&fat_libfs_ops, fat_reg.fs_handle, rid, request);
}

void fat_sync(ipc_callid_t rid, ipc_call_t *request)
{
	/* Dummy implementation */
	ipc_answer_0(rid, EOK);
}

/**
 * @}
 */
