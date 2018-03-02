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
 * @file	exfat_ops.c
 * @brief	Implementation of VFS operations for the exFAT file system
 *		server.
 */

#include "exfat.h"
#include "exfat_fat.h"
#include "exfat_dentry.h"
#include "exfat_directory.h"
#include "exfat_bitmap.h"
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
#include <adt/hash.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <align.h>
#include <stdio.h>
#include <stdlib.h>

/** Mutex protecting the list of cached free FAT nodes. */
static FIBRIL_MUTEX_INITIALIZE(ffn_mutex);

/** List of cached free FAT nodes. */
static LIST_INITIALIZE(ffn_list);

/*
 * Forward declarations of FAT libfs operations.
 */

static errno_t exfat_root_get(fs_node_t **, service_id_t);
static errno_t exfat_match(fs_node_t **, fs_node_t *, const char *);
static errno_t exfat_node_get(fs_node_t **, service_id_t, fs_index_t);
static errno_t exfat_node_open(fs_node_t *);
/* static errno_t exfat_node_put(fs_node_t *); */
static errno_t exfat_create_node(fs_node_t **, service_id_t, int);
static errno_t exfat_destroy_node(fs_node_t *);
static errno_t exfat_link(fs_node_t *, fs_node_t *, const char *);
static errno_t exfat_unlink(fs_node_t *, fs_node_t *, const char *);
static errno_t exfat_has_children(bool *, fs_node_t *);
static fs_index_t exfat_index_get(fs_node_t *);
static aoff64_t exfat_size_get(fs_node_t *);
static unsigned exfat_lnkcnt_get(fs_node_t *);
static bool exfat_is_directory(fs_node_t *);
static bool exfat_is_file(fs_node_t *node);
static service_id_t exfat_service_get(fs_node_t *node);
static errno_t exfat_size_block(service_id_t, uint32_t *);
static errno_t exfat_total_block_count(service_id_t, uint64_t *);
static errno_t exfat_free_block_count(service_id_t, uint64_t *);

/*
 * Helper functions.
 */
static void exfat_node_initialize(exfat_node_t *node)
{
	fibril_mutex_initialize(&node->lock);
	node->bp = NULL;
	node->idx = NULL;
	node->type = EXFAT_UNKNOW;
	link_initialize(&node->ffn_link);
	node->size = 0;
	node->lnkcnt = 0;
	node->refcnt = 0;
	node->dirty = false;
	node->fragmented = false;
	node->lastc_cached_valid = false;
	node->lastc_cached_value = 0;
	node->currc_cached_valid = false;
	node->currc_cached_bn = 0;
	node->currc_cached_value = 0;
}

static errno_t exfat_node_sync(exfat_node_t *node)
{
	errno_t rc;
	exfat_directory_t di;
	exfat_file_dentry_t df;
	exfat_stream_dentry_t ds;

	if (!(node->type == EXFAT_DIRECTORY || node->type == EXFAT_FILE))
		return EOK;

	if (node->type == EXFAT_DIRECTORY)
		df.attr = EXFAT_ATTR_SUBDIR;
	else
		df.attr = 0;

	ds.firstc = node->firstc;
	if (node->size == 0 && node->firstc == 0) {
		ds.flags = 0;
	} else {
		ds.flags = 1;
		ds.flags |= (!node->fragmented << 1);
	}
	ds.valid_data_size = node->size;
	ds.data_size = node->size;

	exfat_directory_open_parent(&di, node->idx->service_id, node->idx->pfc,
	    node->idx->parent_fragmented);
	rc = exfat_directory_seek(&di, node->idx->pdi);
	if (rc != EOK) {
		(void) exfat_directory_close(&di);
		return rc;
	}

	rc = exfat_directory_sync_file(&di, &df, &ds);
	if (rc != EOK) {
		(void) exfat_directory_close(&di);
		return rc;
	}
	return exfat_directory_close(&di);
}

static errno_t exfat_node_fini_by_service_id(service_id_t service_id)
{
	errno_t rc;

	/*
	 * We are called from fat_unmounted() and assume that there are already
	 * no nodes belonging to this instance with non-zero refcount. Therefore
	 * it is sufficient to clean up only the FAT free node list.
	 */

restart:
	fibril_mutex_lock(&ffn_mutex);
	list_foreach(ffn_list, ffn_link, exfat_node_t, nodep) {
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
			rc = exfat_node_sync(nodep);
			if (rc != EOK)
				return rc;
		}
		nodep->idx->nodep = NULL;
		free(nodep->bp);
		free(nodep);

		/* Need to restart because we changed the ffn_list. */
		goto restart;
	}
	fibril_mutex_unlock(&ffn_mutex);

	return EOK;
}

static errno_t exfat_node_get_new(exfat_node_t **nodepp)
{
	fs_node_t *fn;
	exfat_node_t *nodep;
	errno_t rc;

	fibril_mutex_lock(&ffn_mutex);
	if (!list_empty(&ffn_list)) {
		/* Try to use a cached free node structure. */
		exfat_idx_t *idxp_tmp;
		nodep = list_get_instance(list_first(&ffn_list), exfat_node_t,
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
			rc = exfat_node_sync(nodep);
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
		nodep = (exfat_node_t *)malloc(sizeof(exfat_node_t));
		if (!nodep) {
			free(fn);
			return ENOMEM;
		}
	}
	exfat_node_initialize(nodep);
	fs_node_initialize(fn);
	fn->data = nodep;
	nodep->bp = fn;

	*nodepp = nodep;
	return EOK;
}

static errno_t exfat_node_get_new_by_pos(exfat_node_t **nodepp,
    service_id_t service_id, exfat_cluster_t pfc, unsigned pdi)
{
	exfat_idx_t *idxp = exfat_idx_get_by_pos(service_id, pfc, pdi);
	if (!idxp)
		return ENOMEM;
	if (exfat_node_get_new(nodepp) != EOK)
		return ENOMEM;
	(*nodepp)->idx = idxp;
	idxp->nodep = *nodepp;
	return EOK;
}


/** Internal version of exfat_node_get().
 *
 * @param idxp		Locked index structure.
 */
static errno_t exfat_node_get_core(exfat_node_t **nodepp, exfat_idx_t *idxp)
{
	exfat_dentry_t *d;
	exfat_node_t *nodep = NULL;
	exfat_directory_t di;
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

	rc = exfat_node_get_new(&nodep);
	if (rc != EOK)
		return rc;

	exfat_directory_open_parent(&di, idxp->service_id, idxp->pfc,
	    idxp->parent_fragmented);
	rc = exfat_directory_seek(&di, idxp->pdi);
	if (rc != EOK) {
		(void) exfat_directory_close(&di);
		(void) exfat_node_put(FS_NODE(nodep));
		return rc;
	}
	rc = exfat_directory_get(&di, &d);
	if (rc != EOK) {
		(void) exfat_directory_close(&di);
		(void) exfat_node_put(FS_NODE(nodep));
		return rc;
	}

	switch (exfat_classify_dentry(d)) {
	case EXFAT_DENTRY_FILE:
		nodep->type =
		    (uint16_t_le2host(d->file.attr) & EXFAT_ATTR_SUBDIR) ?
		    EXFAT_DIRECTORY : EXFAT_FILE;
		rc = exfat_directory_next(&di);
		if (rc != EOK) {
			(void) exfat_directory_close(&di);
			(void) exfat_node_put(FS_NODE(nodep));
			return rc;
		}
		rc = exfat_directory_get(&di, &d);
		if (rc != EOK) {
			(void) exfat_directory_close(&di);
			(void) exfat_node_put(FS_NODE(nodep));
			return rc;
		}
		nodep->firstc = uint32_t_le2host(d->stream.firstc);
		nodep->size = uint64_t_le2host(d->stream.data_size);
		nodep->fragmented = (d->stream.flags & 0x02) == 0;
		break;
	case EXFAT_DENTRY_BITMAP:
		nodep->type = EXFAT_BITMAP;
		nodep->firstc = uint32_t_le2host(d->bitmap.firstc);
		nodep->size = uint64_t_le2host(d->bitmap.size);
		nodep->fragmented = true;
		break;
	case EXFAT_DENTRY_UCTABLE:
		nodep->type = EXFAT_UCTABLE;
		nodep->firstc = uint32_t_le2host(d->uctable.firstc);
		nodep->size = uint64_t_le2host(d->uctable.size);
		nodep->fragmented = true;
		break;
	default:
	case EXFAT_DENTRY_SKIP:
	case EXFAT_DENTRY_LAST:
	case EXFAT_DENTRY_FREE:
	case EXFAT_DENTRY_VOLLABEL:
	case EXFAT_DENTRY_GUID:
	case EXFAT_DENTRY_STREAM:
	case EXFAT_DENTRY_NAME:
		(void) exfat_directory_close(&di);
		(void) exfat_node_put(FS_NODE(nodep));
		return ENOENT;
	}

	nodep->lnkcnt = 1;
	nodep->refcnt = 1;

	rc = exfat_directory_close(&di);
	if (rc != EOK) {
		(void) exfat_node_put(FS_NODE(nodep));
		return rc;
	}

	/* Link the idx structure with the node structure. */
	nodep->idx = idxp;
	idxp->nodep = nodep;

	*nodepp = nodep;
	return EOK;
}

errno_t exfat_node_expand(service_id_t service_id, exfat_node_t *nodep,
    exfat_cluster_t clusters)
{
	exfat_bs_t *bs;
	errno_t rc;
	bs = block_bb_get(service_id);

	if (!nodep->fragmented) {
		rc = exfat_bitmap_append_clusters(bs, nodep, clusters);
		if (rc != ENOSPC)
			return rc;
		if (rc == ENOSPC) {
			nodep->fragmented = true;
			nodep->dirty = true;		/* need to sync node */
			rc = exfat_bitmap_replicate_clusters(bs, nodep);
			if (rc != EOK)
				return rc;
		}
	}

	/* If we cant linear expand the node, we should use FAT instead */
	exfat_cluster_t mcl, lcl;

	/* create an independent chain of nclsts clusters in all FATs */
	rc = exfat_alloc_clusters(bs, service_id, clusters, &mcl, &lcl);
	if (rc != EOK)
		return rc;
	rc = exfat_zero_cluster(bs, service_id, mcl);
	if (rc != EOK) {
		(void) exfat_free_clusters(bs, service_id, mcl);
		return rc;
	}
	/*
	 * Append the cluster chain starting in mcl to the end of the
	 * node's cluster chain.
	 */
	rc = exfat_append_clusters(bs, nodep, mcl, lcl);
	if (rc != EOK) {
		(void) exfat_free_clusters(bs, service_id, mcl);
		return rc;
	}

	return EOK;
}

static errno_t exfat_node_shrink(service_id_t service_id, exfat_node_t *nodep,
    aoff64_t size)
{
	exfat_bs_t *bs;
	errno_t rc;
	bs = block_bb_get(service_id);

	if (!nodep->fragmented) {
		exfat_cluster_t clsts, prev_clsts, new_clsts;
		prev_clsts = ROUND_UP(nodep->size, BPC(bs)) / BPC(bs);
		new_clsts =  ROUND_UP(size, BPC(bs)) / BPC(bs);

		assert(new_clsts < prev_clsts);

		clsts = prev_clsts - new_clsts;
		rc = exfat_bitmap_free_clusters(bs, nodep, clsts);
		if (rc != EOK)
			return rc;
	} else {
		/*
		 * The node will be shrunk, clusters will be deallocated.
		 */
		if (size == 0) {
			rc = exfat_chop_clusters(bs, nodep, 0);
			if (rc != EOK)
				return rc;
		} else {
			exfat_cluster_t lastc;
			rc = exfat_cluster_walk(bs, service_id, nodep->firstc,
			    &lastc, NULL, (size - 1) / BPC(bs));
			if (rc != EOK)
				return rc;
			rc = exfat_chop_clusters(bs, nodep, lastc);
			if (rc != EOK)
				return rc;
		}
	}

	nodep->size = size;
	nodep->dirty = true;		/* need to sync node */
	return EOK;
}


/*
 * EXFAT libfs operations.
 */

errno_t exfat_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return exfat_node_get(rfn, service_id, EXFAT_ROOT_IDX);
}

errno_t exfat_bitmap_get(fs_node_t **rfn, service_id_t service_id)
{
	return exfat_node_get(rfn, service_id, EXFAT_BITMAP_IDX);
}

errno_t exfat_uctable_get(fs_node_t **rfn, service_id_t service_id)
{
	return exfat_node_get(rfn, service_id, EXFAT_UCTABLE_IDX);
}


errno_t exfat_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	exfat_node_t *parentp = EXFAT_NODE(pfn);
	char name[EXFAT_FILENAME_LEN + 1];
	exfat_file_dentry_t df;
	exfat_stream_dentry_t ds;
	service_id_t service_id;
	errno_t rc;

	fibril_mutex_lock(&parentp->idx->lock);
	service_id = parentp->idx->service_id;
	fibril_mutex_unlock(&parentp->idx->lock);

	exfat_directory_t di;
	rc = exfat_directory_open(parentp, &di);
	if (rc != EOK)
		return rc;

	while (exfat_directory_read_file(&di, name, EXFAT_FILENAME_LEN, &df,
	    &ds) == EOK) {
		if (str_casecmp(name, component) == 0) {
			/* hit */
			exfat_node_t *nodep;
			aoff64_t o = di.pos %
			    (BPS(di.bs) / sizeof(exfat_dentry_t));
			exfat_idx_t *idx = exfat_idx_get_by_pos(service_id,
				parentp->firstc, di.bnum * DPS(di.bs) + o);
			if (!idx) {
				/*
				 * Can happen if memory is low or if we
				 * run out of 32-bit indices.
				 */
				rc = exfat_directory_close(&di);
				return (rc == EOK) ? ENOMEM : rc;
			}
			rc = exfat_node_get_core(&nodep, idx);
			fibril_mutex_unlock(&idx->lock);
			if (rc != EOK) {
				(void) exfat_directory_close(&di);
				return rc;
			}
			*rfn = FS_NODE(nodep);
			rc = exfat_directory_close(&di);
			if (rc != EOK)
				(void) exfat_node_put(*rfn);
			return rc;
		} else {
			rc = exfat_directory_next(&di);
			if (rc != EOK)
				break;
		}
	}
	(void) exfat_directory_close(&di);
	*rfn = NULL;
	return EOK;
}

/** Instantiate a exFAT in-core node. */
errno_t exfat_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	exfat_node_t *nodep;
	exfat_idx_t *idxp;
	errno_t rc;

	idxp = exfat_idx_get_by_index(service_id, index);
	if (!idxp) {
		*rfn = NULL;
		return EOK;
	}
	/* idxp->lock held */
	rc = exfat_node_get_core(&nodep, idxp);
	fibril_mutex_unlock(&idxp->lock);
	if (rc == EOK)
		*rfn = FS_NODE(nodep);
	return rc;
}

errno_t exfat_node_open(fs_node_t *fn)
{
	/*
	 * Opening a file is stateless, nothing
	 * to be done here.
	 */
	return EOK;
}

errno_t exfat_node_put(fs_node_t *fn)
{
	if (fn == NULL)
		return EOK;

	exfat_node_t *nodep = EXFAT_NODE(fn);
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

errno_t exfat_create_node(fs_node_t **rfn, service_id_t service_id, int flags)
{
	exfat_idx_t *idxp;
	exfat_node_t *nodep;
	exfat_bs_t *bs;
	errno_t rc;

	bs = block_bb_get(service_id);
	rc = exfat_node_get_new(&nodep);
	if (rc != EOK)
		return rc;

	rc = exfat_idx_get_new(&idxp, service_id);
	if (rc != EOK) {
		(void) exfat_node_put(FS_NODE(nodep));
		return rc;
	}

	nodep->firstc = 0;
	nodep->size = 0;
	nodep->fragmented = false;
	nodep->lnkcnt = 0;	/* not linked anywhere */
	nodep->refcnt = 1;
	nodep->dirty = true;

	nodep->idx = idxp;
	idxp->nodep = nodep;
	fibril_mutex_unlock(&idxp->lock);

	if (flags & L_DIRECTORY) {
		nodep->type = EXFAT_DIRECTORY;
		rc = exfat_node_expand(service_id, nodep, 1);
		if (rc != EOK) {
			(void) exfat_node_put(FS_NODE(nodep));
			return rc;
		}

		rc = exfat_zero_cluster(bs, service_id, nodep->firstc);
		if (rc != EOK) {
			(void) exfat_node_put(FS_NODE(nodep));
			return rc;
		}

		nodep->size = BPC(bs);
	} else {
		nodep->type = EXFAT_FILE;
	}

	*rfn = FS_NODE(nodep);
	return EOK;
}

errno_t exfat_destroy_node(fs_node_t *fn)
{
	exfat_node_t *nodep = EXFAT_NODE(fn);
	exfat_bs_t *bs;
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
	rc = exfat_has_children(&has_children, fn);
	if (rc != EOK)
		return rc;
	assert(!has_children);

	bs = block_bb_get(nodep->idx->service_id);
	if (nodep->firstc != 0) {
		assert(nodep->size);
		/* Free all clusters allocated to the node. */
		if (nodep->fragmented)
			rc = exfat_free_clusters(bs, nodep->idx->service_id,
				nodep->firstc);
		else
			rc = exfat_bitmap_free_clusters(bs, nodep,
			    ROUND_UP(nodep->size, BPC(bs)) / BPC(bs));
	}

	exfat_idx_destroy(nodep->idx);
	free(nodep->bp);
	free(nodep);
	return rc;
}

errno_t exfat_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	exfat_node_t *parentp = EXFAT_NODE(pfn);
	exfat_node_t *childp = EXFAT_NODE(cfn);
	exfat_directory_t di;
	errno_t rc;

	fibril_mutex_lock(&childp->lock);
	if (childp->lnkcnt == 1) {
		/*
		 * We don't support multiple hard links.
		 */
		fibril_mutex_unlock(&childp->lock);
		return EMLINK;
	}
	assert(childp->lnkcnt == 0);
	fibril_mutex_unlock(&childp->lock);

	if (!exfat_valid_name(name))
		return ENOTSUP;

	fibril_mutex_lock(&parentp->idx->lock);
	rc = exfat_directory_open(parentp, &di);
	if (rc != EOK)
		return rc;
	/*
	 * At this point we only establish the link between the parent and the
	 * child.  The dentry, except of the name and the extension, will remain
	 * uninitialized until the corresponding node is synced. Thus the valid
	 * dentry data is kept in the child node structure.
	 */
	rc = exfat_directory_write_file(&di, name);
	if (rc != EOK) {
		(void) exfat_directory_close(&di);
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}
	rc = exfat_directory_close(&di);
	if (rc != EOK) {
		fibril_mutex_unlock(&parentp->idx->lock);
		return rc;
	}

	fibril_mutex_unlock(&parentp->idx->lock);
	fibril_mutex_lock(&childp->idx->lock);

	childp->idx->pfc = parentp->firstc;
	childp->idx->parent_fragmented = parentp->fragmented;
	childp->idx->pdi = di.pos;
	fibril_mutex_unlock(&childp->idx->lock);

	fibril_mutex_lock(&childp->lock);
	childp->lnkcnt = 1;
	childp->dirty = true;		/* need to sync node */
	fibril_mutex_unlock(&childp->lock);

	/*
	 * Hash in the index structure into the position hash.
	 */
	exfat_idx_hashin(childp->idx);

	return EOK;

}

errno_t exfat_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	exfat_node_t *parentp = EXFAT_NODE(pfn);
	exfat_node_t *childp = EXFAT_NODE(cfn);
	bool has_children;
	errno_t rc;

	if (!parentp)
		return EBUSY;

	rc = exfat_has_children(&has_children, cfn);
	if (rc != EOK)
		return rc;
	if (has_children)
		return ENOTEMPTY;

	fibril_mutex_lock(&parentp->lock);
	fibril_mutex_lock(&childp->lock);
	assert(childp->lnkcnt == 1);
	fibril_mutex_lock(&childp->idx->lock);

	exfat_directory_t di;
	rc = exfat_directory_open(parentp,&di);
	if (rc != EOK)
		goto error;
	rc = exfat_directory_erase_file(&di, childp->idx->pdi);
	if (rc != EOK)
		goto error;
	rc = exfat_directory_close(&di);
	if (rc != EOK)
		goto error;

	/* remove the index structure from the position hash */
	exfat_idx_hashout(childp->idx);
	/* clear position information */
	childp->idx->pfc = 0;
	childp->idx->pdi = 0;
	fibril_mutex_unlock(&childp->idx->lock);
	childp->lnkcnt = 0;
	childp->refcnt++;	/* keep the node in memory until destroyed */
	childp->dirty = true;
	fibril_mutex_unlock(&childp->lock);
	fibril_mutex_unlock(&parentp->lock);

	return EOK;

error:
	(void) exfat_directory_close(&di);
	fibril_mutex_unlock(&childp->idx->lock);
	fibril_mutex_unlock(&childp->lock);
	fibril_mutex_unlock(&parentp->lock);
	return rc;

}

errno_t exfat_has_children(bool *has_children, fs_node_t *fn)
{
	exfat_directory_t di;
	exfat_dentry_t *d;
	exfat_node_t *nodep = EXFAT_NODE(fn);
	errno_t rc;

	*has_children = false;

	if (nodep->type != EXFAT_DIRECTORY)
		return EOK;

	fibril_mutex_lock(&nodep->idx->lock);

	rc = exfat_directory_open(nodep, &di);
	if (rc != EOK) {
		fibril_mutex_unlock(&nodep->idx->lock);
		return rc;
	}

	do {
		rc = exfat_directory_get(&di, &d);
		if (rc != EOK) {
			(void) exfat_directory_close(&di);
			fibril_mutex_unlock(&nodep->idx->lock);
			return rc;
		}
		switch (exfat_classify_dentry(d)) {
		case EXFAT_DENTRY_SKIP:
		case EXFAT_DENTRY_FREE:
			continue;
		case EXFAT_DENTRY_LAST:
			*has_children = false;
			goto exit;
		default:
			*has_children = true;
			goto exit;
		}
	} while (exfat_directory_next(&di) == EOK);

exit:
	rc = exfat_directory_close(&di);
	fibril_mutex_unlock(&nodep->idx->lock);
	return rc;
}


fs_index_t exfat_index_get(fs_node_t *fn)
{
	return EXFAT_NODE(fn)->idx->index;
}

aoff64_t exfat_size_get(fs_node_t *fn)
{
	return EXFAT_NODE(fn)->size;
}

unsigned exfat_lnkcnt_get(fs_node_t *fn)
{
	return EXFAT_NODE(fn)->lnkcnt;
}

bool exfat_is_directory(fs_node_t *fn)
{
	return EXFAT_NODE(fn)->type == EXFAT_DIRECTORY;
}

bool exfat_is_file(fs_node_t *fn)
{
	return EXFAT_NODE(fn)->type == EXFAT_FILE;
}

service_id_t exfat_service_get(fs_node_t *node)
{
	return 0;
}

errno_t exfat_size_block(service_id_t service_id, uint32_t *size)
{
	exfat_bs_t *bs;
	bs = block_bb_get(service_id);
	*size = BPC(bs);

	return EOK;
}

errno_t exfat_total_block_count(service_id_t service_id, uint64_t *count)
{
	exfat_bs_t *bs;
	bs = block_bb_get(service_id);
	*count = DATA_CNT(bs);

	return EOK;
}

errno_t exfat_free_block_count(service_id_t service_id, uint64_t *count)
{
	fs_node_t *node = NULL;
	exfat_node_t *bmap_node;
	exfat_bs_t *bs;
	uint64_t free_block_count = 0;
	uint64_t block_count;
	unsigned sector;
	errno_t rc;

	rc = exfat_total_block_count(service_id, &block_count);
	if (rc != EOK)
		goto exit;

	bs = block_bb_get(service_id);
	rc = exfat_bitmap_get(&node, service_id);
	if (rc != EOK)
		goto exit;

	bmap_node = (exfat_node_t *) node->data;

	unsigned const bmap_sectors = ROUND_UP(bmap_node->size, BPS(bs)) /
	    BPS(bs);

	for (sector = 0; sector < bmap_sectors; ++sector) {

		block_t *block;
		uint8_t *bitmap;
		unsigned bit;

		rc = exfat_block_get(&block, bs, bmap_node, sector,
		    BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			free_block_count = 0;
			goto exit;
		}

		bitmap = (uint8_t *) block->data;

		for (bit = 0; bit < BPS(bs) * 8 && block_count > 0;
		    ++bit, --block_count) {
			if (!(bitmap[bit / 8] & (1 << (bit % 8))))
				++free_block_count;
		}

		block_put(block);

		if (block_count == 0) {
			/* Reached the end of the bitmap */
			goto exit;
		}
	}

exit:
	exfat_node_put(node);
	*count = free_block_count;
	return rc;
}

/** libfs operations */
libfs_ops_t exfat_libfs_ops = {
	.root_get = exfat_root_get,
	.match = exfat_match,
	.node_get = exfat_node_get,
	.node_open = exfat_node_open,
	.node_put = exfat_node_put,
	.create = exfat_create_node,
	.destroy = exfat_destroy_node,
	.link = exfat_link,
	.unlink = exfat_unlink,
	.has_children = exfat_has_children,
	.index_get = exfat_index_get,
	.size_get = exfat_size_get,
	.lnkcnt_get = exfat_lnkcnt_get,
	.is_directory = exfat_is_directory,
	.is_file = exfat_is_file,
	.service_get = exfat_service_get,
	.size_block = exfat_size_block,
	.total_block_count = exfat_total_block_count,
	.free_block_count = exfat_free_block_count
};

static errno_t exfat_fs_open(service_id_t service_id, enum cache_mode cmode,
    fs_node_t **rrfn, exfat_idx_t **rridxp, vfs_fs_probe_info_t *info)
{
	errno_t rc;
	exfat_node_t *rootp = NULL, *bitmapp = NULL, *uctablep = NULL;
	exfat_bs_t *bs;

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

	/* Do some simple sanity checks on the file system. */
	rc = exfat_sanity_check(bs);
	if (rc != EOK) {
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		return rc;
	}

	/* Initialize the block cache */
	rc = block_cache_init(service_id, BPS(bs), 0 /* XXX */, cmode);
	if (rc != EOK) {
		block_fini(service_id);
		return rc;
	}

	rc = exfat_idx_init_by_service_id(service_id);
	if (rc != EOK) {
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		return rc;
	}

	/* Initialize the root node. */
	rc = exfat_node_get_new_by_pos(&rootp, service_id, EXFAT_ROOT_PAR,
	    EXFAT_ROOT_POS);
	if (rc!=EOK) {
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOMEM;
	}
	assert(rootp->idx->index == EXFAT_ROOT_IDX);

	rootp->type = EXFAT_DIRECTORY;
	rootp->firstc = ROOT_FC(bs);
	rootp->fragmented = true;
	rootp->refcnt = 1;
	rootp->lnkcnt = 0;	/* FS root is not linked */

	uint32_t clusters;
	rc = exfat_clusters_get(&clusters, bs, service_id, rootp->firstc);
	if (rc != EOK) {
		free(rootp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOTSUP;
	}
	rootp->size = BPS(bs) * SPC(bs) * clusters;
	fibril_mutex_unlock(&rootp->idx->lock);

	/* Open root directory and looking for Bitmap and UC-Table */
	exfat_directory_t di;
	exfat_dentry_t *de;
	rc = exfat_directory_open(rootp, &di);
	if (rc != EOK) {
		free(rootp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOTSUP;
	}

	/* Initialize the bitmap node. */
	rc = exfat_directory_find(&di, EXFAT_DENTRY_BITMAP, &de);
	if (rc != EOK) {
		free(rootp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOTSUP;
	}

	rc = exfat_node_get_new_by_pos(&bitmapp, service_id, rootp->firstc,
	    di.pos);
	if (rc != EOK) {
		free(rootp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOMEM;
	}
	assert(bitmapp->idx->index == EXFAT_BITMAP_IDX);
	fibril_mutex_unlock(&bitmapp->idx->lock);

	bitmapp->type = EXFAT_BITMAP;
	bitmapp->firstc = uint32_t_le2host(de->bitmap.firstc);
	bitmapp->fragmented = true;
	bitmapp->idx->parent_fragmented = true;
	bitmapp->refcnt = 1;
	bitmapp->lnkcnt = 0;
	bitmapp->size = uint64_t_le2host(de->bitmap.size);

	/* Initialize the uctable node. */
	rc = exfat_directory_seek(&di, 0);
	if (rc != EOK) {
		free(rootp);
		free(bitmapp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOTSUP;
	}

	rc = exfat_directory_find(&di, EXFAT_DENTRY_UCTABLE, &de);
	if (rc != EOK) {
		free(rootp);
		free(bitmapp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOTSUP;
	}

	rc = exfat_node_get_new_by_pos(&uctablep, service_id, rootp->firstc,
	    di.pos);
	if (rc != EOK) {
		free(rootp);
		free(bitmapp);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOMEM;
	}
	assert(uctablep->idx->index == EXFAT_UCTABLE_IDX);
	fibril_mutex_unlock(&uctablep->idx->lock);

	uctablep->type = EXFAT_UCTABLE;
	uctablep->firstc = uint32_t_le2host(de->uctable.firstc);
	uctablep->fragmented = true;
	uctablep->idx->parent_fragmented = true;
	uctablep->refcnt = 1;
	uctablep->lnkcnt = 0;
	uctablep->size = uint64_t_le2host(de->uctable.size);

	if (info != NULL) {
		/* Read volume label. */
		rc = exfat_directory_read_vollabel(&di, info->label,
		    FS_LABEL_MAXLEN + 1);
		if (rc != EOK) {
			free(rootp);
			free(bitmapp);
			free(uctablep);
			(void) block_cache_fini(service_id);
			block_fini(service_id);
			exfat_idx_fini_by_service_id(service_id);
    		    return ENOTSUP;
		}
	}

	rc = exfat_directory_close(&di);
	if (rc != EOK) {
		free(rootp);
		free(bitmapp);
		free(uctablep);
		(void) block_cache_fini(service_id);
		block_fini(service_id);
		exfat_idx_fini_by_service_id(service_id);
		return ENOMEM;
	}

	/* exfat_fsinfo(bs, service_id); */

	*rrfn = FS_NODE(rootp);
	*rridxp = rootp->idx;

	if (info != NULL) {
//		str_cpy(info->label, FS_LABEL_MAXLEN + 1, label);
	}

	return EOK;
}

static void exfat_fs_close(service_id_t service_id, fs_node_t *rfn)
{
	/*
	 * Put the root node and force it to the FAT free node list.
	 */
	(void) exfat_node_put(rfn);
	(void) exfat_node_put(rfn);

	/*
	 * Perform cleanup of the node structures, index structures and
	 * associated data. Write back this file system's dirty blocks and
	 * stop using libblock for this instance.
	 */
	(void) exfat_node_fini_by_service_id(service_id);
	exfat_idx_fini_by_service_id(service_id);
	(void) block_cache_fini(service_id);
	block_fini(service_id);
}


/*
 * VFS_OUT operations.
 */

/* Print debug info */
/*
static void exfat_fsinfo(exfat_bs_t *bs, service_id_t service_id)
{
	printf("exFAT file system mounted\n");
	printf("Version: %d.%d\n", bs->version.major, bs->version.minor);
	printf("Volume serial: %d\n", uint32_t_le2host(bs->volume_serial));
	printf("Volume first sector: %lld\n", VOL_FS(bs));
	printf("Volume sectors: %lld\n", VOL_CNT(bs));
	printf("FAT first sector: %d\n", FAT_FS(bs));
	printf("FAT sectors: %d\n", FAT_CNT(bs));
	printf("Data first sector: %d\n", DATA_FS(bs));
	printf("Data sectors: %d\n", DATA_CNT(bs));
	printf("Root dir first cluster: %d\n", ROOT_FC(bs));
	printf("Bytes per sector: %d\n", BPS(bs));
	printf("Sectors per cluster: %d\n", SPC(bs));
	printf("KBytes per cluster: %d\n", SPC(bs)*BPS(bs)/1024);

	int i, rc;
	exfat_cluster_t clst;
	for (i=0; i<=7; i++) {
		rc = exfat_get_cluster(bs, service_id, i, &clst);
		if (rc != EOK)
			return;
		printf("Clst %d: %x", i, clst);
		if (i>=2)
			printf(", Bitmap: %d\n", bitmap_is_free(bs, service_id, i)!=EOK);
		else
			printf("\n");
	}
}
*/

static errno_t exfat_fsprobe(service_id_t service_id, vfs_fs_probe_info_t *info)
{
	errno_t rc;
	exfat_idx_t *ridxp;
	fs_node_t *rfn;

	rc = exfat_fs_open(service_id, CACHE_MODE_WT, &rfn, &ridxp, info);
	if (rc != EOK)
		return rc;

	exfat_fs_close(service_id, rfn);
	return EOK;
}

static errno_t
exfat_mounted(service_id_t service_id, const char *opts, fs_index_t *index,
    aoff64_t *size)
{
	errno_t rc;
	enum cache_mode cmode;
	exfat_idx_t *ridxp;
	fs_node_t *rfn;

	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;

	rc = exfat_fs_open(service_id, cmode, &rfn, &ridxp, NULL);
	if (rc != EOK)
		return rc;

	*index = ridxp->index;
	*size = EXFAT_NODE(rfn)->size;

	return EOK;
}

static errno_t exfat_unmounted(service_id_t service_id)
{
	fs_node_t *rfn;
	errno_t rc;

	rc = exfat_root_get(&rfn, service_id);
	if (rc != EOK)
		return rc;

	exfat_fs_close(service_id, rfn);
	return EOK;
}

static errno_t
exfat_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	fs_node_t *fn;
	exfat_node_t *nodep;
	exfat_bs_t *bs;
	size_t bytes = 0;
	block_t *b;
	errno_t rc;

	rc = exfat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;
	nodep = EXFAT_NODE(fn);

	ipc_callid_t callid;
	size_t len;
	if (!async_data_read_receive(&callid, &len)) {
		exfat_node_put(fn);
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}

	bs = block_bb_get(service_id);

	if (nodep->type == EXFAT_FILE) {
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
			rc = exfat_block_get(&b, bs, nodep, pos / BPS(bs),
			    BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				exfat_node_put(fn);
				async_answer_0(callid, rc);
				return rc;
			}
			(void) async_data_read_finalize(callid,
			    b->data + pos % BPS(bs), bytes);
			rc = block_put(b);
			if (rc != EOK) {
				exfat_node_put(fn);
				return rc;
			}
		}
	} else {
		if (nodep->type != EXFAT_DIRECTORY) {
			async_answer_0(callid, ENOTSUP);
			return ENOTSUP;
		}

		aoff64_t spos = pos;
		char name[EXFAT_FILENAME_LEN + 1];
		exfat_file_dentry_t df;
		exfat_stream_dentry_t ds;

		assert(nodep->size % BPS(bs) == 0);
		assert(BPS(bs) % sizeof(exfat_dentry_t) == 0);

		exfat_directory_t di;
		rc = exfat_directory_open(nodep, &di);
		if (rc != EOK)
			goto err;

		rc = exfat_directory_seek(&di, pos);
		if (rc != EOK) {
			(void) exfat_directory_close(&di);
			goto err;
		}

		rc = exfat_directory_read_file(&di, name, EXFAT_FILENAME_LEN,
		    &df, &ds);
		if (rc == EOK)
			goto hit;
		else if (rc == ENOENT)
			goto miss;

		(void) exfat_directory_close(&di);

err:
		(void) exfat_node_put(fn);
		async_answer_0(callid, rc);
		return rc;

miss:
		rc = exfat_directory_close(&di);
		if (rc != EOK)
			goto err;
		rc = exfat_node_put(fn);
		async_answer_0(callid, rc != EOK ? rc : ENOENT);
		*rbytes = 0;
		return rc != EOK ? rc : ENOENT;

hit:
		pos = di.pos;
		rc = exfat_directory_close(&di);
		if (rc != EOK)
			goto err;
		(void) async_data_read_finalize(callid, name,
		    str_size(name) + 1);
		bytes = (pos - spos) + 1;
	}

	rc = exfat_node_put(fn);
	*rbytes = bytes;
	return rc;
}

static errno_t exfat_close(service_id_t service_id, fs_index_t index)
{
	return EOK;
}

static errno_t exfat_sync(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	errno_t rc = exfat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;

	exfat_node_t *nodep = EXFAT_NODE(fn);

	nodep->dirty = true;
	rc = exfat_node_sync(nodep);

	exfat_node_put(fn);
	return rc;
}

static errno_t
exfat_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	fs_node_t *fn;
	exfat_node_t *nodep;
	exfat_bs_t *bs;
	size_t bytes;
	block_t *b;
	aoff64_t boundary;
	int flags = BLOCK_FLAGS_NONE;
	errno_t rc;

	rc = exfat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;
	nodep = EXFAT_NODE(fn);

	ipc_callid_t callid;
	size_t len;
	if (!async_data_write_receive(&callid, &len)) {
		(void) exfat_node_put(fn);
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
	if (pos >= boundary) {
		unsigned nclsts;
		nclsts = (ROUND_UP(pos + bytes, BPC(bs)) - boundary) / BPC(bs);
		rc = exfat_node_expand(service_id, nodep, nclsts);
		if (rc != EOK) {
			/* could not expand node */
			(void) exfat_node_put(fn);
			async_answer_0(callid, rc);
			return rc;
		}
	}

	if (pos + bytes > nodep->size) {
		nodep->size = pos + bytes;
		nodep->dirty = true;	/* need to sync node */
	}

	/*
	 * This is the easier case - we are either overwriting already
	 * existing contents or writing behind the EOF, but still within
	 * the limits of the last cluster. The node size may grow to the
	 * next block size boundary.
	 */
	rc = exfat_block_get(&b, bs, nodep, pos / BPS(bs), flags);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		async_answer_0(callid, rc);
		return rc;
	}

	(void) async_data_write_finalize(callid,
	    b->data + pos % BPS(bs), bytes);
	b->dirty = true;		/* need to sync block */
	rc = block_put(b);
	if (rc != EOK) {
		(void) exfat_node_put(fn);
		return rc;
	}

	*wbytes = bytes;
	*nsize = nodep->size;
	rc = exfat_node_put(fn);
	return rc;
}

static errno_t
exfat_truncate(service_id_t service_id, fs_index_t index, aoff64_t size)
{
	fs_node_t *fn;
	exfat_node_t *nodep;
	exfat_bs_t *bs;
	errno_t rc;

	rc = exfat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;
	nodep = EXFAT_NODE(fn);

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
		rc = exfat_node_shrink(service_id, nodep, size);
	}

	errno_t rc2 = exfat_node_put(fn);
	if (rc == EOK && rc2 != EOK)
		rc = rc2;

	return rc;
}

static errno_t exfat_destroy(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	exfat_node_t *nodep;
	errno_t rc;

	rc = exfat_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;

	nodep = EXFAT_NODE(fn);
	/*
	 * We should have exactly two references. One for the above
	 * call to fat_node_get() and one from fat_unlink().
	 */
	assert(nodep->refcnt == 2);

	rc = exfat_destroy_node(fn);
	return rc;
}

vfs_out_ops_t exfat_ops = {
	.fsprobe = exfat_fsprobe,
	.mounted = exfat_mounted,
	.unmounted = exfat_unmounted,
	.read = exfat_read,
	.write = exfat_write,
	.truncate = exfat_truncate,
	.close = exfat_close,
	.destroy = exfat_destroy,
	.sync = exfat_sync,
};

/**
 * @}
 */
