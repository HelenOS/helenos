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
#include <libadt/hash_table.h>
#include <libadt/list.h>
#include <assert.h>
#include <futex.h>
#include <sys/mman.h>
#include <align.h>

/** Futex protecting the list of cached free FAT nodes. */
static futex_t ffn_futex = FUTEX_INITIALIZER;

/** List of cached free FAT nodes. */
static LIST_INITIALIZE(ffn_head);

static void fat_node_initialize(fat_node_t *node)
{
	futex_initialize(&node->lock, 1);
	node->idx = NULL;
	node->type = 0;
	link_initialize(&node->ffn_link);
	node->size = 0;
	node->lnkcnt = 0;
	node->refcnt = 0;
	node->dirty = false;
}

static void fat_node_sync(fat_node_t *node)
{
	block_t *b;
	fat_bs_t *bs;
	fat_dentry_t *d;
	uint16_t bps;
	unsigned dps;
	
	assert(node->dirty);

	bs = block_bb_get(node->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);
	
	/* Read the block that contains the dentry of interest. */
	b = _fat_block_get(bs, node->idx->dev_handle, node->idx->pfc,
	    (node->idx->pdi * sizeof(fat_dentry_t)) / bps, BLOCK_FLAGS_NONE);

	d = ((fat_dentry_t *)b->data) + (node->idx->pdi % dps);

	d->firstc = host2uint16_t_le(node->firstc);
	if (node->type == FAT_FILE) {
		d->size = host2uint32_t_le(node->size);
	} else if (node->type == FAT_DIRECTORY) {
		d->attr = FAT_ATTR_SUBDIR;
	}
	
	/* TODO: update other fields? (e.g time fields) */
	
	b->dirty = true;		/* need to sync block */
	block_put(b);
}

static fat_node_t *fat_node_get_new(void)
{
	fat_node_t *nodep;

	futex_down(&ffn_futex);
	if (!list_empty(&ffn_head)) {
		/* Try to use a cached free node structure. */
		fat_idx_t *idxp_tmp;
		nodep = list_get_instance(ffn_head.next, fat_node_t, ffn_link);
		if (futex_trydown(&nodep->lock) == ESYNCH_WOULD_BLOCK)
			goto skip_cache;
		idxp_tmp = nodep->idx;
		if (futex_trydown(&idxp_tmp->lock) == ESYNCH_WOULD_BLOCK) {
			futex_up(&nodep->lock);
			goto skip_cache;
		}
		list_remove(&nodep->ffn_link);
		futex_up(&ffn_futex);
		if (nodep->dirty)
			fat_node_sync(nodep);
		idxp_tmp->nodep = NULL;
		futex_up(&nodep->lock);
		futex_up(&idxp_tmp->lock);
	} else {
skip_cache:
		/* Try to allocate a new node structure. */
		futex_up(&ffn_futex);
		nodep = (fat_node_t *)malloc(sizeof(fat_node_t));
		if (!nodep)
			return NULL;
	}
	fat_node_initialize(nodep);
	
	return nodep;
}

/** Internal version of fat_node_get().
 *
 * @param idxp		Locked index structure.
 */
static void *fat_node_get_core(fat_idx_t *idxp)
{
	block_t *b;
	fat_bs_t *bs;
	fat_dentry_t *d;
	fat_node_t *nodep = NULL;
	unsigned bps;
	unsigned spc;
	unsigned dps;

	if (idxp->nodep) {
		/*
		 * We are lucky.
		 * The node is already instantiated in memory.
		 */
		futex_down(&idxp->nodep->lock);
		if (!idxp->nodep->refcnt++)
			list_remove(&idxp->nodep->ffn_link);
		futex_up(&idxp->nodep->lock);
		return idxp->nodep;
	}

	/*
	 * We must instantiate the node from the file system.
	 */
	
	assert(idxp->pfc);

	nodep = fat_node_get_new();
	if (!nodep)
		return NULL;

	bs = block_bb_get(idxp->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	spc = bs->spc;
	dps = bps / sizeof(fat_dentry_t);

	/* Read the block that contains the dentry of interest. */
	b = _fat_block_get(bs, idxp->dev_handle, idxp->pfc,
	    (idxp->pdi * sizeof(fat_dentry_t)) / bps, BLOCK_FLAGS_NONE);
	assert(b);

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
		nodep->size = bps * spc * fat_clusters_get(bs, idxp->dev_handle,
		    uint16_t_le2host(d->firstc));
	} else {
		nodep->type = FAT_FILE;
		nodep->size = uint32_t_le2host(d->size);
	}
	nodep->firstc = uint16_t_le2host(d->firstc);
	nodep->lnkcnt = 1;
	nodep->refcnt = 1;

	block_put(b);

	/* Link the idx structure with the node structure. */
	nodep->idx = idxp;
	idxp->nodep = nodep;

	return nodep;
}

/*
 * Forward declarations of FAT libfs operations.
 */
static void *fat_node_get(dev_handle_t, fs_index_t);
static void fat_node_put(void *);
static void *fat_create_node(dev_handle_t, int);
static int fat_destroy_node(void *);
static int fat_link(void *, void *, const char *);
static int fat_unlink(void *, void *);
static void *fat_match(void *, const char *);
static fs_index_t fat_index_get(void *);
static size_t fat_size_get(void *);
static unsigned fat_lnkcnt_get(void *);
static bool fat_has_children(void *);
static void *fat_root_get(dev_handle_t);
static char fat_plb_get_char(unsigned);
static bool fat_is_directory(void *);
static bool fat_is_file(void *node);

/*
 * FAT libfs operations.
 */

/** Instantiate a FAT in-core node. */
void *fat_node_get(dev_handle_t dev_handle, fs_index_t index)
{
	void *node;
	fat_idx_t *idxp;

	idxp = fat_idx_get_by_index(dev_handle, index);
	if (!idxp)
		return NULL;
	/* idxp->lock held */
	node = fat_node_get_core(idxp);
	futex_up(&idxp->lock);
	return node;
}

void fat_node_put(void *node)
{
	fat_node_t *nodep = (fat_node_t *)node;
	bool destroy = false;

	futex_down(&nodep->lock);
	if (!--nodep->refcnt) {
		if (nodep->idx) {
			futex_down(&ffn_futex);
			list_append(&nodep->ffn_link, &ffn_head);
			futex_up(&ffn_futex);
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
	futex_up(&nodep->lock);
	if (destroy)
		free(node);
}

void *fat_create_node(dev_handle_t dev_handle, int flags)
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
			return NULL;
	}

	nodep = fat_node_get_new();
	if (!nodep) {
		fat_free_clusters(bs, dev_handle, mcl);	
		return NULL;
	}
	idxp = fat_idx_get_new(dev_handle);
	if (!idxp) {
		fat_free_clusters(bs, dev_handle, mcl);	
		fat_node_put(nodep);
		return NULL;
	}
	/* idxp->lock held */
	if (flags & L_DIRECTORY) {
		int i;
		block_t *b;

		/*
		 * Populate the new cluster with unused dentries.
		 */
		for (i = 0; i < bs->spc; i++) {
			b = _fat_block_get(bs, dev_handle, mcl, i,
			    BLOCK_FLAGS_NOREAD);
			/* mark all dentries as never-used */
			memset(b->data, 0, bps);
			b->dirty = false;
			block_put(b);
		}
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

	futex_up(&idxp->lock);
	return nodep;
}

int fat_destroy_node(void *node)
{
	fat_node_t *nodep = (fat_node_t *)node;
	fat_bs_t *bs;

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
	assert(fat_has_children(node) == false);

	bs = block_bb_get(nodep->idx->dev_handle);
	if (nodep->firstc != FAT_CLST_RES0) {
		assert(nodep->size);
		/* Free all clusters allocated to the node. */
		fat_free_clusters(bs, nodep->idx->dev_handle, nodep->firstc);
	}

	fat_idx_destroy(nodep->idx);
	free(nodep);
	return EOK;
}

int fat_link(void *prnt, void *chld, const char *name)
{
	fat_node_t *parentp = (fat_node_t *)prnt;
	fat_node_t *childp = (fat_node_t *)chld;
	fat_dentry_t *d;
	fat_bs_t *bs;
	block_t *b;
	int i, j;
	uint16_t bps;
	unsigned dps;
	unsigned blocks;
	fat_cluster_t mcl, lcl;
	int rc;

	futex_down(&childp->lock);
	if (childp->lnkcnt == 1) {
		/*
		 * On FAT, we don't support multiple hard links.
		 */
		futex_up(&childp->lock);
		return EMLINK;
	}
	assert(childp->lnkcnt == 0);
	futex_up(&childp->lock);

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
	
	futex_down(&parentp->idx->lock);
	bs = block_bb_get(parentp->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);

	blocks = parentp->size / bps;

	for (i = 0; i < blocks; i++) {
		b = fat_block_get(bs, parentp, i, BLOCK_FLAGS_NONE);
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
		block_put(b);
	}
	j = 0;
	
	/*
	 * We need to grow the parent in order to create a new unused dentry.
	 */
	if (parentp->idx->pfc == FAT_CLST_ROOT) {
		/* Can't grow the root directory. */
		futex_up(&parentp->idx->lock);
		return ENOSPC;
	}
	rc = fat_alloc_clusters(bs, parentp->idx->dev_handle, 1, &mcl, &lcl);
	if (rc != EOK) {
		futex_up(&parentp->idx->lock);
		return rc;
	}
	fat_append_clusters(bs, parentp, mcl);
	b = fat_block_get(bs, parentp, i, BLOCK_FLAGS_NOREAD);
	d = (fat_dentry_t *)b->data;
	/*
	 * Clear all dentries in the block except for the first one (the first
	 * dentry will be cleared in the next step).
	 */
	memset(d + 1, 0, bps - sizeof(fat_dentry_t));

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
	block_put(b);
	futex_up(&parentp->idx->lock);

	futex_down(&childp->idx->lock);
	
	/*
	 * If possible, create the Sub-directory Identifier Entry and the
	 * Sub-directory Parent Pointer Entry (i.e. "." and ".."). These entries
	 * are not mandatory according to Standard ECMA-107 and HelenOS VFS does
	 * not use them anyway, so this is rather a sign of our good will.
	 */
	b = fat_block_get(bs, childp, 0, BLOCK_FLAGS_NONE);
	d = (fat_dentry_t *)b->data;
	if (fat_classify_dentry(d) == FAT_DENTRY_LAST ||
	    str_cmp(d->name, FAT_NAME_DOT) == 0) {
	   	memset(d, 0, sizeof(fat_dentry_t));
	   	strcpy(d->name, FAT_NAME_DOT);
		strcpy(d->ext, FAT_EXT_PAD);
		d->attr = FAT_ATTR_SUBDIR;
		d->firstc = host2uint16_t_le(childp->firstc);
		/* TODO: initialize also the date/time members. */
	}
	d++;
	if (fat_classify_dentry(d) == FAT_DENTRY_LAST ||
	    str_cmp(d->name, FAT_NAME_DOT_DOT) == 0) {
		memset(d, 0, sizeof(fat_dentry_t));
		strcpy(d->name, FAT_NAME_DOT_DOT);
		strcpy(d->ext, FAT_EXT_PAD);
		d->attr = FAT_ATTR_SUBDIR;
		d->firstc = (parentp->firstc == FAT_CLST_ROOT) ?
		    host2uint16_t_le(FAT_CLST_RES0) :
		    host2uint16_t_le(parentp->firstc);
		/* TODO: initialize also the date/time members. */
	}
	b->dirty = true;		/* need to sync block */
	block_put(b);

	childp->idx->pfc = parentp->firstc;
	childp->idx->pdi = i * dps + j;
	futex_up(&childp->idx->lock);

	futex_down(&childp->lock);
	childp->lnkcnt = 1;
	childp->dirty = true;		/* need to sync node */
	futex_up(&childp->lock);

	/*
	 * Hash in the index structure into the position hash.
	 */
	fat_idx_hashin(childp->idx);

	return EOK;
}

int fat_unlink(void *prnt, void *chld)
{
	fat_node_t *parentp = (fat_node_t *)prnt;
	fat_node_t *childp = (fat_node_t *)chld;
	fat_bs_t *bs;
	fat_dentry_t *d;
	uint16_t bps;
	block_t *b;

	futex_down(&parentp->lock);
	futex_down(&childp->lock);
	assert(childp->lnkcnt == 1);
	futex_down(&childp->idx->lock);
	bs = block_bb_get(childp->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);

	b = _fat_block_get(bs, childp->idx->dev_handle, childp->idx->pfc,
	    (childp->idx->pdi * sizeof(fat_dentry_t)) / bps,
	    BLOCK_FLAGS_NONE);
	d = (fat_dentry_t *)b->data +
	    (childp->idx->pdi % (bps / sizeof(fat_dentry_t)));
	/* mark the dentry as not-currently-used */
	d->name[0] = FAT_DENTRY_ERASED;
	b->dirty = true;		/* need to sync block */
	block_put(b);

	/* remove the index structure from the position hash */
	fat_idx_hashout(childp->idx);
	/* clear position information */
	childp->idx->pfc = FAT_CLST_RES0;
	childp->idx->pdi = 0;
	futex_up(&childp->idx->lock);
	childp->lnkcnt = 0;
	childp->dirty = true;
	futex_up(&childp->lock);
	futex_up(&parentp->lock);

	return EOK;
}

void *fat_match(void *prnt, const char *component)
{
	fat_bs_t *bs;
	fat_node_t *parentp = (fat_node_t *)prnt;
	char name[FAT_NAME_LEN + 1 + FAT_EXT_LEN + 1];
	unsigned i, j;
	unsigned bps;		/* bytes per sector */
	unsigned dps;		/* dentries per sector */
	unsigned blocks;
	fat_dentry_t *d;
	block_t *b;

	futex_down(&parentp->idx->lock);
	bs = block_bb_get(parentp->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);
	blocks = parentp->size / bps;
	for (i = 0; i < blocks; i++) {
		b = fat_block_get(bs, parentp, i, BLOCK_FLAGS_NONE);
		for (j = 0; j < dps; j++) { 
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_FREE:
				continue;
			case FAT_DENTRY_LAST:
				block_put(b);
				futex_up(&parentp->idx->lock);
				return NULL;
			default:
			case FAT_DENTRY_VALID:
				fat_dentry_name_get(d, name);
				break;
			}
			if (fat_dentry_namecmp(name, component) == 0) {
				/* hit */
				void *node;
				/*
				 * Assume tree hierarchy for locking.  We
				 * already have the parent and now we are going
				 * to lock the child.  Never lock in the oposite
				 * order.
				 */
				fat_idx_t *idx = fat_idx_get_by_pos(
				    parentp->idx->dev_handle, parentp->firstc,
				    i * dps + j);
				futex_up(&parentp->idx->lock);
				if (!idx) {
					/*
					 * Can happen if memory is low or if we
					 * run out of 32-bit indices.
					 */
					block_put(b);
					return NULL;
				}
				node = fat_node_get_core(idx);
				futex_up(&idx->lock);
				block_put(b);
				return node;
			}
		}
		block_put(b);
	}

	futex_up(&parentp->idx->lock);
	return NULL;
}

fs_index_t fat_index_get(void *node)
{
	fat_node_t *fnodep = (fat_node_t *)node;
	if (!fnodep)
		return 0;
	return fnodep->idx->index;
}

size_t fat_size_get(void *node)
{
	return ((fat_node_t *)node)->size;
}

unsigned fat_lnkcnt_get(void *node)
{
	return ((fat_node_t *)node)->lnkcnt;
}

bool fat_has_children(void *node)
{
	fat_bs_t *bs;
	fat_node_t *nodep = (fat_node_t *)node;
	unsigned bps;
	unsigned dps;
	unsigned blocks;
	block_t *b;
	unsigned i, j;

	if (nodep->type != FAT_DIRECTORY)
		return false;
	
	futex_down(&nodep->idx->lock);
	bs = block_bb_get(nodep->idx->dev_handle);
	bps = uint16_t_le2host(bs->bps);
	dps = bps / sizeof(fat_dentry_t);

	blocks = nodep->size / bps;

	for (i = 0; i < blocks; i++) {
		fat_dentry_t *d;
	
		b = fat_block_get(bs, nodep, i, BLOCK_FLAGS_NONE);
		for (j = 0; j < dps; j++) {
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
			case FAT_DENTRY_FREE:
				continue;
			case FAT_DENTRY_LAST:
				block_put(b);
				futex_up(&nodep->idx->lock);
				return false;
			default:
			case FAT_DENTRY_VALID:
				block_put(b);
				futex_up(&nodep->idx->lock);
				return true;
			}
			block_put(b);
			futex_up(&nodep->idx->lock);
			return true;
		}
		block_put(b);
	}

	futex_up(&nodep->idx->lock);
	return false;
}

void *fat_root_get(dev_handle_t dev_handle)
{
	return fat_node_get(dev_handle, 0);
}

char fat_plb_get_char(unsigned pos)
{
	return fat_reg.plb_ro[pos % PLB_SIZE];
}

bool fat_is_directory(void *node)
{
	return ((fat_node_t *)node)->type == FAT_DIRECTORY;
}

bool fat_is_file(void *node)
{
	return ((fat_node_t *)node)->type == FAT_FILE;
}

/** libfs operations */
libfs_ops_t fat_libfs_ops = {
	.match = fat_match,
	.node_get = fat_node_get,
	.node_put = fat_node_put,
	.create = fat_create_node,
	.destroy = fat_destroy_node,
	.link = fat_link,
	.unlink = fat_unlink,
	.index_get = fat_index_get,
	.size_get = fat_size_get,
	.lnkcnt_get = fat_lnkcnt_get,
	.has_children = fat_has_children,
	.root_get = fat_root_get,
	.plb_get_char =	fat_plb_get_char,
	.is_directory = fat_is_directory,
	.is_file = fat_is_file
};

/*
 * VFS operations.
 */

void fat_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	fat_bs_t *bs;
	uint16_t bps;
	uint16_t rde;
	int rc;

	/* initialize libblock */
	rc = block_init(dev_handle, BS_SIZE);
	if (rc != EOK) {
		ipc_answer_0(rid, rc);
		return;
	}

	/* prepare the boot block */
	rc = block_bb_read(dev_handle, BS_BLOCK * BS_SIZE, BS_SIZE);
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
	rc = block_cache_init(dev_handle, bps, 0 /* XXX */);
	if (rc != EOK) {
		block_fini(dev_handle);
		ipc_answer_0(rid, rc);
		return;
	}

	rc = fat_idx_init_by_dev_handle(dev_handle);
	if (rc != EOK) {
		block_fini(dev_handle);
		ipc_answer_0(rid, rc);
		return;
	}

	/* Initialize the root node. */
	fat_node_t *rootp = (fat_node_t *)malloc(sizeof(fat_node_t));
	if (!rootp) {
		block_fini(dev_handle);
		fat_idx_fini_by_dev_handle(dev_handle);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	fat_node_initialize(rootp);

	fat_idx_t *ridxp = fat_idx_get_by_pos(dev_handle, FAT_CLST_ROOTPAR, 0);
	if (!ridxp) {
		block_fini(dev_handle);
		free(rootp);
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
	
	futex_up(&ridxp->lock);

	ipc_answer_3(rid, EOK, ridxp->index, rootp->size, rootp->lnkcnt);
}

void fat_mount(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
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
	fat_node_t *nodep = (fat_node_t *)fat_node_get(dev_handle, index);
	fat_bs_t *bs;
	uint16_t bps;
	size_t bytes;
	block_t *b;

	if (!nodep) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	ipc_callid_t callid;
	size_t len;
	if (!ipc_data_read_receive(&callid, &len)) {
		fat_node_put(nodep);
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
			(void) ipc_data_read_finalize(callid, NULL, 0);
		} else {
			bytes = min(len, bps - pos % bps);
			bytes = min(bytes, nodep->size - pos);
			b = fat_block_get(bs, nodep, pos / bps,
			    BLOCK_FLAGS_NONE);
			(void) ipc_data_read_finalize(callid, b->data + pos % bps,
			    bytes);
			block_put(b);
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

			b = fat_block_get(bs, nodep, bnum, BLOCK_FLAGS_NONE);
			for (o = pos % (bps / sizeof(fat_dentry_t));
			    o < bps / sizeof(fat_dentry_t);
			    o++, pos++) {
				d = ((fat_dentry_t *)b->data) + o;
				switch (fat_classify_dentry(d)) {
				case FAT_DENTRY_SKIP:
				case FAT_DENTRY_FREE:
					continue;
				case FAT_DENTRY_LAST:
					block_put(b);
					goto miss;
				default:
				case FAT_DENTRY_VALID:
					fat_dentry_name_get(d, name);
					block_put(b);
					goto hit;
				}
			}
			block_put(b);
			bnum++;
		}
miss:
		fat_node_put(nodep);
		ipc_answer_0(callid, ENOENT);
		ipc_answer_1(rid, ENOENT, 0);
		return;
hit:
		(void) ipc_data_read_finalize(callid, name, str_size(name) + 1);
		bytes = (pos - spos) + 1;
	}

	fat_node_put(nodep);
	ipc_answer_1(rid, EOK, (ipcarg_t)bytes);
}

void fat_write(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	off_t pos = (off_t)IPC_GET_ARG3(*request);
	fat_node_t *nodep = (fat_node_t *)fat_node_get(dev_handle, index);
	fat_bs_t *bs;
	size_t bytes;
	block_t *b;
	uint16_t bps;
	unsigned spc;
	unsigned bpc;		/* bytes per cluster */
	off_t boundary;
	int flags = BLOCK_FLAGS_NONE;
	
	if (!nodep) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	
	ipc_callid_t callid;
	size_t len;
	if (!ipc_data_write_receive(&callid, &len)) {
		fat_node_put(nodep);
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
		fat_fill_gap(bs, nodep, FAT_CLST_RES0, pos);
		b = fat_block_get(bs, nodep, pos / bps, flags);
		(void) ipc_data_write_finalize(callid, b->data + pos % bps,
		    bytes);
		b->dirty = true;		/* need to sync block */
		block_put(b);
		if (pos + bytes > nodep->size) {
			nodep->size = pos + bytes;
			nodep->dirty = true;	/* need to sync node */
		}
		ipc_answer_2(rid, EOK, bytes, nodep->size);	
		fat_node_put(nodep);
		return;
	} else {
		/*
		 * This is the more difficult case. We must allocate new
		 * clusters for the node and zero them out.
		 */
		int status;
		unsigned nclsts;
		fat_cluster_t mcl, lcl; 
 
		nclsts = (ROUND_UP(pos + bytes, bpc) - boundary) / bpc;
		/* create an independent chain of nclsts clusters in all FATs */
		status = fat_alloc_clusters(bs, dev_handle, nclsts, &mcl, &lcl);
		if (status != EOK) {
			/* could not allocate a chain of nclsts clusters */
			fat_node_put(nodep);
			ipc_answer_0(callid, status);
			ipc_answer_0(rid, status);
			return;
		}
		/* zero fill any gaps */
		fat_fill_gap(bs, nodep, mcl, pos);
		b = _fat_block_get(bs, dev_handle, lcl, (pos / bps) % spc,
		    flags);
		(void) ipc_data_write_finalize(callid, b->data + pos % bps,
		    bytes);
		b->dirty = true;		/* need to sync block */
		block_put(b);
		/*
		 * Append the cluster chain starting in mcl to the end of the
		 * node's cluster chain.
		 */
		fat_append_clusters(bs, nodep, mcl);
		nodep->size = pos + bytes;
		nodep->dirty = true;		/* need to sync node */
		ipc_answer_2(rid, EOK, bytes, nodep->size);
		fat_node_put(nodep);
		return;
	}
}

void fat_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	size_t size = (off_t)IPC_GET_ARG3(*request);
	fat_node_t *nodep = (fat_node_t *)fat_node_get(dev_handle, index);
	fat_bs_t *bs;
	uint16_t bps;
	uint8_t spc;
	unsigned bpc;	/* bytes per cluster */
	int rc;

	if (!nodep) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

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
			fat_chop_clusters(bs, nodep, FAT_CLST_RES0);
		} else {
			fat_cluster_t lastc;
			(void) fat_cluster_walk(bs, dev_handle, nodep->firstc,
			    &lastc, (size - 1) / bpc);
			fat_chop_clusters(bs, nodep, lastc);
		}
		nodep->size = size;
		nodep->dirty = true;		/* need to sync node */
		rc = EOK;	
	}
	fat_node_put(nodep);
	ipc_answer_0(rid, rc);
	return;
}

void fat_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	int rc;

	fat_node_t *nodep = fat_node_get(dev_handle, index);
	if (!nodep) {
		ipc_answer_0(rid, ENOENT);
		return;
	}

	rc = fat_destroy_node(nodep);
	ipc_answer_0(rid, rc);
}

/**
 * @}
 */ 
