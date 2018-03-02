/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Oleg Romanenko
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
#include "fat_directory.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <block.h>
#include <ipc/services.h>
#include <ipc/loc.h>
#include <macros.h>
#include <async.h>
#include <errno.h>
#include <str.h>
#include <byteorder.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <align.h>
#include <stdlib.h>

#define FAT_NODE(node)	((node) ? (fat_node_t *) (node)->data : NULL)
#define FS_NODE(node)	((node) ? (node)->bp : NULL)

#define DPS(bs)		(BPS((bs)) / sizeof(fat_dentry_t))
#define BPC(bs)		(BPS((bs)) * SPC((bs)))

/** Mutex protecting the list of cached free FAT nodes. */
static FIBRIL_MUTEX_INITIALIZE(ffn_mutex);

/** List of cached free FAT nodes. */
static LIST_INITIALIZE(ffn_list);

/*
 * Forward declarations of FAT libfs operations.
 */
static errno_t fat_root_get(fs_node_t **, service_id_t);
static errno_t fat_match(fs_node_t **, fs_node_t *, const char *);
static errno_t fat_node_get(fs_node_t **, service_id_t, fs_index_t);
static errno_t fat_node_open(fs_node_t *);
static errno_t fat_node_put(fs_node_t *);
static errno_t fat_create_node(fs_node_t **, service_id_t, int);
static errno_t fat_destroy_node(fs_node_t *);
static errno_t fat_link(fs_node_t *, fs_node_t *, const char *);
static errno_t fat_unlink(fs_node_t *, fs_node_t *, const char *);
static errno_t fat_has_children(bool *, fs_node_t *);
static fs_index_t fat_index_get(fs_node_t *);
static aoff64_t fat_size_get(fs_node_t *);
static unsigned fat_lnkcnt_get(fs_node_t *);
static bool fat_is_directory(fs_node_t *);
static bool fat_is_file(fs_node_t *node);
static service_id_t fat_service_get(fs_node_t *node);
static errno_t fat_size_block(service_id_t, uint32_t *);
static errno_t fat_total_block_count(service_id_t, uint64_t *);
static errno_t fat_free_block_count(service_id_t, uint64_t *);

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
	node->lastc_cached_valid = false;
	node->lastc_cached_value = 0;
	node->currc_cached_valid = false;
	node->currc_cached_bn = 0;
	node->currc_cached_value = 0;
}

static errno_t fat_node_sync(fat_node_t *node)
{
	block_t *b;
	fat_bs_t *bs;
	fat_dentry_t *d;
	errno_t rc;

	assert(node->dirty);

	bs = block_bb_get(node->idx->service_id);

	/* Read the block that contains the dentry of interest. */
	rc = _fat_block_get(&b, bs, node->idx->service_id, node->idx->pfc,
	    NULL, (node->idx->pdi * sizeof(fat_dentry_t)) / BPS(bs),
	    BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	d = ((fat_dentry_t *)b->data) + (node->idx->pdi % DPS(bs));

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

static errno_t fat_node_fini_by_service_id(service_id_t service_id)
{
	errno_t rc;

	/*
	 * We are called from fat_unmounted() and assume that there are already
	 * no nodes belonging to this instance with non-zero refcount. Therefore
	 * it is sufficient to clean up only the FAT free node list.
	 */

restart:
	fibril_mutex_lock(&ffn_mutex);
	list_foreach(ffn_list, ffn_link, fat_node_t, nodep) {
		if (!fibril_mutex_trylock(&nodep->lock)) {
			fibril_mutex_unlock(&ffn_mutex);
			goto restart;
		}
		if (!fibril_mutex_trylock(&nodep->idx->lock)) {
			fibril_mutex_unlock(&nodep->lock);
			fibril_mutex_unlock(&ffn_mutex);
			goto restart;
		}
		if (nodep->idx->service_id != service_id) {
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

		/* Need to restart because we changed ffn_list. */
		goto restart;
	}
	fibril_mutex_unlock(&ffn_mutex);

	return EOK;
}

static errno_t fat_node_get_new(fat_node_t **nodepp)
{
	fs_node_t *fn;
	fat_node_t *nodep;
	errno_t rc;

	fibril_mutex_lock(&ffn_mutex);
	if (!list_empty(&ffn_list)) {
		/* Try to use a cached free node structure. */
		fat_idx_t *idxp_tmp;
		nodep = list_get_instance(list_first(&ffn_list), fat_node_t,
		    ffn_link);
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
static errno_t fat_node_get_core(fat_node_t **nodepp, fat_idx_t *idxp)
{
	block_t *b;
	fat_bs_t *bs;
	fat_dentry_t *d;
	fat_node_t *nodep = NULL;
	errno_t rc;

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

	bs = block_bb_get(idxp->service_id);

	/* Read the block that contains the dentry of interest. */
	rc = _fat_block_get(&b, bs, idxp->service_id, idxp->pfc, NULL,
	    (idxp->pdi * sizeof(fat_dentry_t)) / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		(void) fat_node_put(FS_NODE(nodep));
		return rc;
	}

	d = ((fat_dentry_t *)b->data) + (idxp->pdi % DPS(bs));
	if (FAT_IS_FAT32(bs)) {
		nodep->firstc = uint16_t_le2host(d->firstc_lo) |
		    (uint16_t_le2host(d->firstc_hi) << 16);
	} else
		nodep->firstc = uint16_t_le2host(d->firstc);

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
		uint32_t clusters;
		rc = fat_clusters_get(&clusters, bs, idxp->service_id,
		    nodep->firstc);
		if (rc != EOK) {
			(void) block_put(b);
			(void) fat_node_put(FS_NODE(nodep));
			return rc;
		}
		nodep->size = BPS(bs) * SPC(bs) * clusters;
	} else {
		nodep->type = FAT_FILE;
		nodep->size = uint32_t_le2host(d->size);
	}

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

errno_t fat_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return fat_node_get(rfn, service_id, 0);
}

errno_t fat_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	fat_node_t *parentp = FAT_NODE(pfn);
	char name[FAT_LFN_NAME_SIZE];
	fat_dentry_t *d;
	service_id_t service_id;
	errno_t rc;

	fibril_mutex_lock(&parentp->idx->lock);
	service_id = parentp->idx->service_id;
	fibril_mutex_unlock(&parentp->idx->lock);

	fat_directory_t di;
	rc = fat_directory_open(parentp, &di);
	if (rc != EOK)
		return rc;

	while (fat_directory_read(&di, name, &d) == EOK) {
		if (fat_dentry_namecmp(name, component) == 0) {
			/* hit */
			fat_node_t *nodep;
			aoff64_t o = di.pos %
			    (BPS(di.bs) / sizeof(fat_dentry_t));
			fat_idx_t *idx = fat_idx_get_by_pos(service_id,
			    parentp->firstc, di.bnum * DPS(di.bs) + o);
			if (!idx) {
				/*
				 * Can happen if memory is low or if we
				 * run out of 32-bit indices.
				 */
				rc = fat_directory_close(&di);
				return (rc == EOK) ? ENOMEM : rc;
			}
			rc = fat_node_get_core(&nodep, idx);
			fibril_mutex_unlock(&idx->lock);
			if (rc != EOK) {
				(void) fat_directory_close(&di);
				return rc;
			}
			*rfn = FS_NODE(nodep);
			rc = fat_directory_close(&di);
			if (rc != EOK)
				(void) fat_node_put(*rfn);
			return rc;
		} else {
			rc = fat_directory_next(&di);
			if (rc != EOK)
				break;
		}
	}
	(void) fat_directory_close(&di);
	*rfn = NULL;
	return EOK;
}

/** Instantiate a FAT in-core node. */
errno_t fat_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	fat_node_t *nodep;
	fat_idx_t *idxp;
	errno_t rc;

	idxp = fat_idx_get_by_index(service_id, index);
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

errno_t fat_node_open(fs_node_t *fn)
{
	/*
	 * Opening a file is stateless, nothing
	 * to be done here.
	 */
	return EOK;
}

errno_t fat_node_put(fs_node_t *fn)
{
	fat_node_t *nodep = FAT_NODE(fn);
	bool destroy = false;

	fibril_mutex_lock(&nodep->lock);
	if (!--nodep->refcnt) {
		if (nodep->idx) {
			fibril_mutex_lock(&ffn_mutex);
			list_append(&nodep->ffn_link, &ffn_list);
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

errno_t fat_create_node(fs_node_t **rfn, service_id_t service_id, int flags)
{
	fat_idx_t *idxp;
	fat_node_t *nodep;
	fat_bs_t *bs;
	fat_cluster_t mcl, lcl;
	errno_t rc;

	bs = block_bb_get(service_id);
	if (flags & L_DIRECTORY) {
		/* allocate a cluster */
		rc = fat_alloc_clusters(bs, service_id, 1, &mcl, &lcl);
		if (rc != EOK)
			return rc;
		/* populate the new cluster with unused dentries */
		rc = fat_zero_cluster(bs, service_id, mcl);
		if (rc != EOK)
			goto error;
	}

	rc = fat_node_get_new(&nodep);
	if (rc != EOK)
		goto error;
	rc = fat_idx_get_new(&idxp, service_id);
	if (rc != EOK) {
		(void) fat_node_put(FS_NODE(nodep));
		goto error;
	}
	/* idxp->lock held */
	if (flags & L_DIRECTORY) {
		nodep->type = FAT_DIRECTORY;
		nodep->firstc = mcl;
		nodep->size = BPS(bs) * SPC(bs);
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

error:
	if (flags & L_DIRECTORY)
		(void) fat_free_clusters(bs, service_id, mcl);
	return rc;
}

errno_t fat_destroy_node(fs_node_t *fn)
{
	fat_node_t *nodep = FAT_NODE(fn);
	fat_bs_t *bs;
	bool has_children;
	errno_t rc;

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

	bs = block_bb_get(nodep->idx->service_id);
	if (nodep->firstc != FAT_CLST_RES0) {
		assert(nodep->size);
		/* Free all clusters allocated to the node. */
		rc = fat_free_clusters(bs, nodep->idx->service_id,
		    nodep->firstc);
	}

	fat_idx_destroy(nodep->idx);
	free(nodep->bp);
	free(nodep);
	return rc;
}

errno_t fat_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	fat_node_t *parentp = FAT_NODE(pfn);
	fat_node_t *childp = FAT_NODE(cfn);
	fat_dentry_t *d;
	fat_bs_t *bs;
	block_t *b;
	fat_directory_t di;
	fat_dentry_t de;
	errno_t rc;

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

	if (!fat_valid_name(name))
		return ENOTSUP;

	fibril_mutex_lock(&parentp->idx->lock);
	bs = block_bb_get(parentp->idx->service_id);
	rc = fat_directory_open(parentp, &di);
	if (rc != EOK) {
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}

	/*
	 * At this point we only establish the link between the parent and the
	 * child.  The dentry, except of the name and the extension, will remain
	 * uninitialized until the corresponding node is synced. Thus the valid
	 * dentry data is kept in the child node structure.
	 */
	memset(&de, 0, sizeof(fat_dentry_t));

	rc = fat_directory_write(&di, name, &de);
	if (rc != EOK) {
		(void) fat_directory_close(&di);
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}
	rc = fat_directory_close(&di);
	if (rc != EOK) {
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}

	fibril_mutex_unlock(&parentp->idx->lock);

	fibril_mutex_lock(&childp->idx->lock);

	if (childp->type == FAT_DIRECTORY) {
		/*
		 * If possible, create the Sub-directory Identifier Entry and
		 * the Sub-directory Parent Pointer Entry (i.e. "." and "..").
		 * These entries are not mandatory according to Standard
		 * ECMA-107 and HelenOS VFS does not use them anyway, so this is
		 * rather a sign of our good will.
		 */
		rc = fat_block_get(&b, bs, childp, 0, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			/*
			 * Rather than returning an error, simply skip the
			 * creation of these two entries.
			 */
			goto skip_dots;
		}
		d = (fat_dentry_t *) b->data;
		if ((fat_classify_dentry(d) == FAT_DENTRY_LAST) ||
		    (memcmp(d->name, FAT_NAME_DOT, FAT_NAME_LEN)) == 0) {
			memset(d, 0, sizeof(fat_dentry_t));
			memcpy(d->name, FAT_NAME_DOT, FAT_NAME_LEN);
			memcpy(d->ext, FAT_EXT_PAD, FAT_EXT_LEN);
			d->attr = FAT_ATTR_SUBDIR;
			d->firstc = host2uint16_t_le(childp->firstc);
			/* TODO: initialize also the date/time members. */
		}
		d++;
		if ((fat_classify_dentry(d) == FAT_DENTRY_LAST) ||
		    (memcmp(d->name, FAT_NAME_DOT_DOT, FAT_NAME_LEN) == 0)) {
			memset(d, 0, sizeof(fat_dentry_t));
			memcpy(d->name, FAT_NAME_DOT_DOT, FAT_NAME_LEN);
			memcpy(d->ext, FAT_EXT_PAD, FAT_EXT_LEN);
			d->attr = FAT_ATTR_SUBDIR;
			d->firstc = (parentp->firstc == FAT_ROOT_CLST(bs)) ?
			    host2uint16_t_le(FAT_CLST_ROOTPAR) :
			    host2uint16_t_le(parentp->firstc);
			/* TODO: initialize also the date/time members. */
		}
		b->dirty = true;		/* need to sync block */
		/*
		 * Ignore the return value as we would have fallen through on error
		 * anyway.
		 */
		(void) block_put(b);
	}
skip_dots:

	childp->idx->pfc = parentp->firstc;
	childp->idx->pdi = di.pos;	/* di.pos holds absolute position of SFN entry */
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

errno_t fat_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	fat_node_t *parentp = FAT_NODE(pfn);
	fat_node_t *childp = FAT_NODE(cfn);
	bool has_children;
	errno_t rc;

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

	fat_directory_t di;
	rc = fat_directory_open(parentp, &di);
	if (rc != EOK)
		goto error;
	rc = fat_directory_seek(&di, childp->idx->pdi);
	if (rc != EOK)
		goto error;
	rc = fat_directory_erase(&di);
	if (rc != EOK)
		goto error;
	rc = fat_directory_close(&di);
	if (rc != EOK)
		goto error;

	/* remove the index structure from the position hash */
	fat_idx_hashout(childp->idx);
	/* clear position information */
	childp->idx->pfc = FAT_CLST_RES0;
	childp->idx->pdi = 0;
	fibril_mutex_unlock(&childp->idx->lock);
	childp->lnkcnt = 0;
	childp->refcnt++;	/* keep the node in memory until destroyed */
	childp->dirty = true;
	fibril_mutex_unlock(&childp->lock);
	fibril_mutex_unlock(&parentp->lock);

	return EOK;

error:
	(void) fat_directory_close(&di);
	fibril_mutex_unlock(&childp->idx->lock);
	fibril_mutex_unlock(&childp->lock);
	fibril_mutex_unlock(&parentp->lock);
	return rc;
}

errno_t fat_has_children(bool *has_children, fs_node_t *fn)
{
	fat_bs_t *bs;
	fat_node_t *nodep = FAT_NODE(fn);
	unsigned blocks;
	block_t *b;
	unsigned i, j;
	errno_t rc;

	if (nodep->type != FAT_DIRECTORY) {
		*has_children = false;
		return EOK;
	}

	fibril_mutex_lock(&nodep->idx->lock);
	bs = block_bb_get(nodep->idx->service_id);

	blocks = nodep->size / BPS(bs);

	for (i = 0; i < blocks; i++) {
		fat_dentry_t *d;

		rc = fat_block_get(&b, bs, nodep, i, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			fibril_mutex_unlock(&nodep->idx->lock);
			return rc;
		}
		for (j = 0; j < DPS(bs); j++) {
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_FREE:
			case FAT_DENTRY_VOLLABEL:
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

aoff64_t fat_size_get(fs_node_t *fn)
{
	return FAT_NODE(fn)->size;
}

unsigned fat_lnkcnt_get(fs_node_t *fn)
{
	return FAT_NODE(fn)->lnkcnt;
}

bool fat_is_directory(fs_node_t *fn)
{
	return FAT_NODE(fn)->type == FAT_DIRECTORY;
}

bool fat_is_file(fs_node_t *fn)
{
	return FAT_NODE(fn)->type == FAT_FILE;
}

service_id_t fat_service_get(fs_node_t *fn)
{
	return FAT_NODE(fn)->idx->service_id;
}

errno_t fat_size_block(service_id_t service_id, uint32_t *size)
{
	fat_bs_t *bs;

	bs = block_bb_get(service_id);
	*size = BPC(bs);

	return EOK;
}

errno_t fat_total_block_count(service_id_t service_id, uint64_t *count)
{
	fat_bs_t *bs;

	bs = block_bb_get(service_id);
	*count = (SPC(bs)) ? TS(bs) / SPC(bs) : 0;

	return EOK;
}

errno_t fat_free_block_count(service_id_t service_id, uint64_t *count)
{
	fat_bs_t *bs;
	fat_cluster_t e0;
	uint64_t block_count;
	errno_t rc;
	uint32_t cluster_no, clusters;

	block_count = 0;
	bs = block_bb_get(service_id);
	clusters = (SPC(bs)) ? TS(bs) / SPC(bs) : 0;
	for (cluster_no = 0; cluster_no < clusters; cluster_no++) {
		rc = fat_get_cluster(bs, service_id, FAT1, cluster_no, &e0);
		if (rc != EOK)
			return EIO;

		if (e0 == FAT_CLST_RES0)
			block_count++;
	}
	*count = block_count;

	return EOK;
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
	.is_directory = fat_is_directory,
	.is_file = fat_is_file,
	.service_get = fat_service_get,
	.size_block = fat_size_block,
	.total_block_count = fat_total_block_count,
	.free_block_count = fat_free_block_count
};

static errno_t fat_fs_open(service_id_t service_id, enum cache_mode cmode,
    fs_node_t **rrfn, fat_idx_t **rridxp)
{
	fat_bs_t *bs;
	errno_t rc;

	/* initialize libblock */
	rc = block_init(service_id, BS_SIZE);
	if (rc != EOK)
		return rc;

	/* prepare the boot block */
	rc = block_bb_read(service_id, BS_BLOCK);
	if (rc != EOK) {
		block_fini(service_id);
		return rc;
	}

	/* get the buffer with the boot sector */
	bs = block_bb_get(service_id);

	if (BPS(bs) != BS_SIZE) {
		block_fini(service_id);
		return ENOTSUP;
	}

	/* Initialize the block cache */
	rc = block_cache_init(service_id, BPS(bs), 0 /* XXX */, cmode);
	if (rc != EOK) {
		block_fini(service_id);
		return rc;
	}

	/* Do some simple sanity checks on the file system. */
	rc = fat_sanity_check(bs, service_id);
	if (rc != EOK) {
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		return rc;
	}

	rc = fat_idx_init_by_service_id(service_id);
	if (rc != EOK) {
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		return rc;
	}

	/* Initialize the root node. */
	fs_node_t *rfn = (fs_node_t *)malloc(sizeof(fs_node_t));
	if (!rfn) {
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		fat_idx_fini_by_service_id(service_id);
		return ENOMEM;
	}

	fs_node_initialize(rfn);
	fat_node_t *rootp = (fat_node_t *)malloc(sizeof(fat_node_t));
	if (!rootp) {
		free(rfn);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		fat_idx_fini_by_service_id(service_id);
		return ENOMEM;
	}
	fat_node_initialize(rootp);

	fat_idx_t *ridxp = fat_idx_get_by_pos(service_id, FAT_CLST_ROOTPAR, 0);
	if (!ridxp) {
		free(rfn);
		free(rootp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		fat_idx_fini_by_service_id(service_id);
		return ENOMEM;
	}
	assert(ridxp->index == 0);
	/* ridxp->lock held */

	rootp->type = FAT_DIRECTORY;
	rootp->firstc = FAT_ROOT_CLST(bs);
	rootp->refcnt = 1;
	rootp->lnkcnt = 0;	/* FS root is not linked */

	if (FAT_IS_FAT32(bs)) {
		uint32_t clusters;
		rc = fat_clusters_get(&clusters, bs, service_id, rootp->firstc);
		if (rc != EOK) {
			fibril_mutex_unlock(&ridxp->lock);
			free(rfn);
			free(rootp);
			(void) block_cache_fini(service_id);
			block_fini(service_id);
			fat_idx_fini_by_service_id(service_id);
			return ENOTSUP;
		}
		rootp->size = BPS(bs) * SPC(bs) * clusters;
	} else
		rootp->size = RDE(bs) * sizeof(fat_dentry_t);

	rootp->idx = ridxp;
	ridxp->nodep = rootp;
	rootp->bp = rfn;
	rfn->data = rootp;

	fibril_mutex_unlock(&ridxp->lock);

	*rrfn = rfn;
	*rridxp = ridxp;

	return EOK;
}

static void fat_fs_close(service_id_t service_id, fs_node_t *rfn)
{
	free(rfn->data);
	free(rfn);
	(void) block_cache_fini(service_id);
	block_fini(service_id);
	fat_idx_fini_by_service_id(service_id);
}

/*
 * FAT VFS_OUT operations.
 */

static errno_t fat_fsprobe(service_id_t service_id, vfs_fs_probe_info_t *info)
{
	fat_idx_t *ridxp;
	fs_node_t *rfn;
	fat_node_t *nodep;
	fat_directory_t di;
	char label[FAT_VOLLABEL_LEN + 1];
	errno_t rc;

	rc = fat_fs_open(service_id, CACHE_MODE_WT, &rfn, &ridxp);
	if (rc != EOK)
		return rc;

	nodep = FAT_NODE(rfn);

	rc = fat_directory_open(nodep, &di);
	if (rc != EOK) {
		fat_fs_close(service_id, rfn);
		return rc;
	}

	rc = fat_directory_vollabel_get(&di, label);
	if (rc != EOK) {
		if (rc != ENOENT) {
			fat_fs_close(service_id, rfn);
			return rc;
		}

		label[0] = '\0';
	}

	str_cpy(info->label, FS_LABEL_MAXLEN + 1, label);

	fat_directory_close(&di);
	fat_fs_close(service_id, rfn);

	return EOK;
}

static errno_t
fat_mounted(service_id_t service_id, const char *opts, fs_index_t *index,
    aoff64_t *size)
{
	enum cache_mode cmode = CACHE_MODE_WB;
	fat_instance_t *instance;
	fat_idx_t *ridxp;
	fs_node_t *rfn;
	errno_t rc;

	instance = malloc(sizeof(fat_instance_t));
	if (!instance)
		return ENOMEM;
	instance->lfn_enabled = true;

	/* Parse mount options. */
	char *mntopts = (char *) opts;
	char *opt;
	while ((opt = str_tok(mntopts, " ,", &mntopts)) != NULL) {
		if (str_cmp(opt, "wtcache") == 0)
			cmode = CACHE_MODE_WT;
		else if (str_cmp(opt, "nolfn") == 0)
			instance->lfn_enabled = false;
	}

	rc = fat_fs_open(service_id, cmode, &rfn, &ridxp);
	if (rc != EOK) {
		free(instance);
		return rc;
	}

	fibril_mutex_lock(&ridxp->lock);

	rc = fs_instance_create(service_id, instance);
	if (rc != EOK) {
		fibril_mutex_unlock(&ridxp->lock);
		fat_fs_close(service_id, rfn);
		free(instance);
		return rc;
	}

	fibril_mutex_unlock(&ridxp->lock);

	*index = ridxp->index;
	*size = FAT_NODE(rfn)->size;

	return EOK;
}

static errno_t fat_update_fat32_fsinfo(service_id_t service_id)
{
	fat_bs_t *bs;
	fat32_fsinfo_t *info;
	block_t *b;
	errno_t rc;

	bs = block_bb_get(service_id);
	assert(FAT_IS_FAT32(bs));

	rc = block_get(&b, service_id, uint16_t_le2host(bs->fat32.fsinfo_sec),
	    BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	info = (fat32_fsinfo_t *) b->data;

	if (memcmp(info->sig1, FAT32_FSINFO_SIG1, sizeof(info->sig1)) != 0 ||
	    memcmp(info->sig2, FAT32_FSINFO_SIG2, sizeof(info->sig2)) != 0 ||
	    memcmp(info->sig3, FAT32_FSINFO_SIG3, sizeof(info->sig3)) != 0) {
		(void) block_put(b);
		return EINVAL;
	}

	/* For now, invalidate the counter. */
	info->free_clusters = host2uint16_t_le(-1);

	b->dirty = true;
	return block_put(b);
}

static errno_t fat_unmounted(service_id_t service_id)
{
	fs_node_t *fn;
	fat_node_t *nodep;
	fat_bs_t *bs;
	errno_t rc;

	bs = block_bb_get(service_id);

	rc = fat_root_get(&fn, service_id);
	if (rc != EOK)
		return rc;
	nodep = FAT_NODE(fn);

	/*
	 * We expect exactly two references on the root node. One for the
	 * fat_root_get() above and one created in fat_mounted().
	 */
	if (nodep->refcnt != 2) {
		(void) fat_node_put(fn);
		return EBUSY;
	}

	if (FAT_IS_FAT32(bs)) {
		/*
		 * Attempt to update the FAT32 FS info.
		 */
		(void) fat_update_fat32_fsinfo(service_id);
	}

	/*
	 * Put the root node and force it to the FAT free node list.
	 */
	(void) fat_node_put(fn);

	/*
	 * Perform cleanup of the node structures, index structures and
	 * associated data. Write back this file system's dirty blocks and
	 * stop using libblock for this instance.
	 */
	(void) fat_node_fini_by_service_id(service_id);
	fat_fs_close(service_id, fn);

	void *data;
	if (fs_instance_get(service_id, &data) == EOK) {
		fs_instance_destroy(service_id);
		free(data);
	}

	return EOK;
}

static errno_t
fat_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	fs_node_t *fn;
	fat_node_t *nodep;
	fat_bs_t *bs;
	size_t bytes;
	block_t *b;
	errno_t rc;

	rc = fat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;
	nodep = FAT_NODE(fn);

	ipc_callid_t callid;
	size_t len;
	if (!async_data_read_receive(&callid, &len)) {
		fat_node_put(fn);
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}

	bs = block_bb_get(service_id);

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
			bytes = min(len, BPS(bs) - pos % BPS(bs));
			bytes = min(bytes, nodep->size - pos);
			rc = fat_block_get(&b, bs, nodep, pos / BPS(bs),
			    BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				fat_node_put(fn);
				async_answer_0(callid, rc);
				return rc;
			}
			(void) async_data_read_finalize(callid,
			    b->data + pos % BPS(bs), bytes);
			rc = block_put(b);
			if (rc != EOK) {
				fat_node_put(fn);
				return rc;
			}
		}
	} else {
		aoff64_t spos = pos;
		char name[FAT_LFN_NAME_SIZE];
		fat_dentry_t *d;

		assert(nodep->type == FAT_DIRECTORY);
		assert(nodep->size % BPS(bs) == 0);
		assert(BPS(bs) % sizeof(fat_dentry_t) == 0);

		fat_directory_t di;
		rc = fat_directory_open(nodep, &di);
		if (rc != EOK)
			goto err;
		rc = fat_directory_seek(&di, pos);
		if (rc != EOK) {
			(void) fat_directory_close(&di);
			goto err;
		}

		rc = fat_directory_read(&di, name, &d);
		if (rc == EOK)
			goto hit;
		if (rc == ENOENT)
			goto miss;

err:
		(void) fat_node_put(fn);
		async_answer_0(callid, rc);
		return rc;

miss:
		rc = fat_directory_close(&di);
		if (rc != EOK)
			goto err;
		rc = fat_node_put(fn);
		async_answer_0(callid, rc != EOK ? rc : ENOENT);
		*rbytes = 0;
		return rc != EOK ? rc : ENOENT;

hit:
		pos = di.pos;
		rc = fat_directory_close(&di);
		if (rc != EOK)
			goto err;
		(void) async_data_read_finalize(callid, name,
		    str_size(name) + 1);
		bytes = (pos - spos) + 1;
	}

	rc = fat_node_put(fn);
	*rbytes = bytes;
	return rc;
}

static errno_t
fat_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	fs_node_t *fn;
	fat_node_t *nodep;
	fat_bs_t *bs;
	size_t bytes;
	block_t *b;
	aoff64_t boundary;
	int flags = BLOCK_FLAGS_NONE;
	errno_t rc;

	rc = fat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;
	nodep = FAT_NODE(fn);

	ipc_callid_t callid;
	size_t len;
	if (!async_data_write_receive(&callid, &len)) {
		(void) fat_node_put(fn);
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}

	bs = block_bb_get(service_id);

	/*
	 * In all scenarios, we will attempt to write out only one block worth
	 * of data at maximum. There might be some more efficient approaches,
	 * but this one greatly simplifies fat_write(). Note that we can afford
	 * to do this because the client must be ready to handle the return
	 * value signalizing a smaller number of bytes written.
	 */
	bytes = min(len, BPS(bs) - pos % BPS(bs));
	if (bytes == BPS(bs))
		flags |= BLOCK_FLAGS_NOREAD;

	boundary = ROUND_UP(nodep->size, BPC(bs));
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
			async_answer_0(callid, rc);
			return rc;
		}
		rc = fat_block_get(&b, bs, nodep, pos / BPS(bs), flags);
		if (rc != EOK) {
			(void) fat_node_put(fn);
			async_answer_0(callid, rc);
			return rc;
		}
		(void) async_data_write_finalize(callid,
		    b->data + pos % BPS(bs), bytes);
		b->dirty = true;		/* need to sync block */
		rc = block_put(b);
		if (rc != EOK) {
			(void) fat_node_put(fn);
			return rc;
		}
		if (pos + bytes > nodep->size) {
			nodep->size = pos + bytes;
			nodep->dirty = true;	/* need to sync node */
		}
		*wbytes = bytes;
		*nsize = nodep->size;
		rc = fat_node_put(fn);
		return rc;
	} else {
		/*
		 * This is the more difficult case. We must allocate new
		 * clusters for the node and zero them out.
		 */
		unsigned nclsts;
		fat_cluster_t mcl, lcl;

		nclsts = (ROUND_UP(pos + bytes, BPC(bs)) - boundary) / BPC(bs);
		/* create an independent chain of nclsts clusters in all FATs */
		rc = fat_alloc_clusters(bs, service_id, nclsts, &mcl, &lcl);
		if (rc != EOK) {
			/* could not allocate a chain of nclsts clusters */
			(void) fat_node_put(fn);
			async_answer_0(callid, rc);
			return rc;
		}
		/* zero fill any gaps */
		rc = fat_fill_gap(bs, nodep, mcl, pos);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, service_id, mcl);
			(void) fat_node_put(fn);
			async_answer_0(callid, rc);
			return rc;
		}
		rc = _fat_block_get(&b, bs, service_id, lcl, NULL,
		    (pos / BPS(bs)) % SPC(bs), flags);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, service_id, mcl);
			(void) fat_node_put(fn);
			async_answer_0(callid, rc);
			return rc;
		}
		(void) async_data_write_finalize(callid,
		    b->data + pos % BPS(bs), bytes);
		b->dirty = true;		/* need to sync block */
		rc = block_put(b);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, service_id, mcl);
			(void) fat_node_put(fn);
			return rc;
		}
		/*
		 * Append the cluster chain starting in mcl to the end of the
		 * node's cluster chain.
		 */
		rc = fat_append_clusters(bs, nodep, mcl, lcl);
		if (rc != EOK) {
			(void) fat_free_clusters(bs, service_id, mcl);
			(void) fat_node_put(fn);
			return rc;
		}
		*nsize = nodep->size = pos + bytes;
		rc = fat_node_put(fn);
		nodep->dirty = true;		/* need to sync node */
		*wbytes = bytes;
		return rc;
	}
}

static errno_t
fat_truncate(service_id_t service_id, fs_index_t index, aoff64_t size)
{
	fs_node_t *fn;
	fat_node_t *nodep;
	fat_bs_t *bs;
	errno_t rc;

	rc = fat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;
	nodep = FAT_NODE(fn);

	bs = block_bb_get(service_id);

	if (nodep->size == size) {
		rc = EOK;
	} else if (nodep->size < size) {
		/*
		 * The standard says we have the freedom to grow the node.
		 * For now, we simply return an error.
		 */
		rc = EINVAL;
	} else if (ROUND_UP(nodep->size, BPC(bs)) == ROUND_UP(size, BPC(bs))) {
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
			rc = fat_cluster_walk(bs, service_id, nodep->firstc,
			    &lastc, NULL, (size - 1) / BPC(bs));
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
	return rc;
}

static errno_t fat_close(service_id_t service_id, fs_index_t index)
{
	return EOK;
}

static errno_t fat_destroy(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	fat_node_t *nodep;
	errno_t rc;

	rc = fat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;

	nodep = FAT_NODE(fn);
	/*
	 * We should have exactly two references. One for the above
	 * call to fat_node_get() and one from fat_unlink().
	 */
	assert(nodep->refcnt == 2);

	rc = fat_destroy_node(fn);
	return rc;
}

static errno_t fat_sync(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	errno_t rc = fat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;

	fat_node_t *nodep = FAT_NODE(fn);

	nodep->dirty = true;
	rc = fat_node_sync(nodep);

	fat_node_put(fn);
	return rc;
}

vfs_out_ops_t fat_ops = {
	.fsprobe = fat_fsprobe,
	.mounted = fat_mounted,
	.unmounted = fat_unmounted,
	.read = fat_read,
	.write = fat_write,
	.truncate = fat_truncate,
	.close = fat_close,
	.destroy = fat_destroy,
	.sync = fat_sync,
};

/**
 * @}
 */
