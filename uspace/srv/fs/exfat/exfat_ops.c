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
 * @brief	Implementation of VFS operations for the exFAT file system server.
 */

#include "exfat.h"
#include "exfat_fat.h"
#include "exfat_dentry.h"
#include "exfat_directory.h"
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <libblock.h>
#include <ipc/services.h>
#include <ipc/devmap.h>
#include <macros.h>
#include <async.h>
#include <errno.h>
#include <str.h>
#include <byteorder.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <sys/mman.h>
#include <align.h>
#include <malloc.h>
#include <stdio.h>

#define EXFAT_NODE(node)	((node) ? (exfat_node_t *) (node)->data : NULL)
#define FS_NODE(node)	((node) ? (node)->bp : NULL)


/** Mutex protecting the list of cached free FAT nodes. */
static FIBRIL_MUTEX_INITIALIZE(ffn_mutex);

/** List of cached free FAT nodes. */
static LIST_INITIALIZE(ffn_head);

/*
 * Forward declarations of FAT libfs operations.
 */
/*
static int exfat_bitmap_get(fs_node_t **, devmap_handle_t);
static int exfat_uctable_get(fs_node_t **, devmap_handle_t);
*/
static int exfat_root_get(fs_node_t **, devmap_handle_t);
static int exfat_match(fs_node_t **, fs_node_t *, const char *);
static int exfat_node_get(fs_node_t **, devmap_handle_t, fs_index_t);
static int exfat_node_open(fs_node_t *);
static int exfat_node_put(fs_node_t *);
static int exfat_create_node(fs_node_t **, devmap_handle_t, int);
static int exfat_destroy_node(fs_node_t *);
static int exfat_link(fs_node_t *, fs_node_t *, const char *);
static int exfat_unlink(fs_node_t *, fs_node_t *, const char *);
static int exfat_has_children(bool *, fs_node_t *);
static fs_index_t exfat_index_get(fs_node_t *);
static aoff64_t exfat_size_get(fs_node_t *);
static unsigned exfat_lnkcnt_get(fs_node_t *);
static char exfat_plb_get_char(unsigned);
static bool exfat_is_directory(fs_node_t *);
static bool exfat_is_file(fs_node_t *node);
static devmap_handle_t exfat_device_get(fs_node_t *node);

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
	node->fragmented = true;
	node->lastc_cached_valid = false;
	node->lastc_cached_value = 0;
	node->currc_cached_valid = false;
	node->currc_cached_bn = 0;
	node->currc_cached_value = 0;
}

static int exfat_node_sync(exfat_node_t *node)
{
	return EOK;
}

static int exfat_node_fini_by_devmap_handle(devmap_handle_t devmap_handle)
{
	link_t *lnk;
	exfat_node_t *nodep;
	int rc;

	/*
	 * We are called from fat_unmounted() and assume that there are already
	 * no nodes belonging to this instance with non-zero refcount. Therefore
	 * it is sufficient to clean up only the FAT free node list.
	 */

restart:
	fibril_mutex_lock(&ffn_mutex);
	for (lnk = ffn_head.next; lnk != &ffn_head; lnk = lnk->next) {
		nodep = list_get_instance(lnk, exfat_node_t, ffn_link);
		if (!fibril_mutex_trylock(&nodep->lock)) {
			fibril_mutex_unlock(&ffn_mutex);
			goto restart;
		}
		if (!fibril_mutex_trylock(&nodep->idx->lock)) {
			fibril_mutex_unlock(&nodep->lock);
			fibril_mutex_unlock(&ffn_mutex);
			goto restart;
		}
		if (nodep->idx->devmap_handle != devmap_handle) {
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

		/* Need to restart because we changed the ffn_head list. */
		goto restart;
	}
	fibril_mutex_unlock(&ffn_mutex);

	return EOK;
}

static int exfat_node_get_new(exfat_node_t **nodepp)
{
	fs_node_t *fn;
	exfat_node_t *nodep;
	int rc;

	fibril_mutex_lock(&ffn_mutex);
	if (!list_empty(&ffn_head)) {
		/* Try to use a cached free node structure. */
		exfat_idx_t *idxp_tmp;
		nodep = list_get_instance(ffn_head.next, exfat_node_t, ffn_link);
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

static int exfat_node_get_new_by_pos(exfat_node_t **nodepp, 
    devmap_handle_t devmap_handle, exfat_cluster_t pfc, unsigned pdi)
{
	exfat_idx_t *idxp = exfat_idx_get_by_pos(devmap_handle, pfc, pdi);
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
static int exfat_node_get_core(exfat_node_t **nodepp, exfat_idx_t *idxp)
{
	block_t *b=NULL;
	//exfat_bs_t *bs;
	//exfat_dentry_t *d;
	exfat_node_t *nodep = NULL;
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

	rc = exfat_node_get_new(&nodep);
	if (rc != EOK)
		return rc;

	//bs = block_bb_get(idxp->devmap_handle);

	/* Access to exFAT directory and read two entries:
	 * file entry and stream entry 
	 */
	/*
	exfat_directory_t di;
	exfat_dentry_t *de;
	exfat_directory_open(&di, ???);
	exfat_directory_seek(&di, idxp->pdi);
	exfat_directory_get(&di, &de); 

	switch (exfat_classify_dentry(de)) {
	case EXFAT_DENTRY_FILE:
		nodep->type = (de->file.attr & EXFAT_ATTR_SUBDIR)? 
		    EXFAT_DIRECTORY : EXFAT_FILE;
		exfat_directory_next(&di);
		exfat_directory_get(&di, &de);
		nodep->firtsc = de->stream.firstc;
		nodep->size = de->stream.data_size;
		nodep->fragmented = (de->stream.flags & 0x02) == 0;
		break;
	case EXFAT_DENTRY_BITMAP:
		nodep->type = EXFAT_BITMAP;
		nodep->firstc = de->bitmap.firstc;
		nodep->size = de->bitmap.size;
		nodep->fragmented = false;
		break;
	case EXFAT_DENTRY_UCTABLE:
		nodep->type = EXFAT_UCTABLE;
		nodep->firstc = de->uctable.firstc;
		nodep->size = de->uctable.size;
		nodep->fragmented = false;
		break;
	default:
	case EXFAT_DENTRY_SKIP:
	case EXFAT_DENTRY_LAST:
	case EXFAT_DENTRY_FREE:
	case EXFAT_DENTRY_VOLLABEL:
	case EXFAT_DENTRY_GUID:
	case EXFAT_DENTRY_STREAM:
	case EXFAT_DENTRY_NAME:
		(void) block_put(b);
		(void) fat_node_put(FS_NODE(nodep));
		return ENOENT;
	}
	*/

	/* Read the block that contains the dentry of interest. */
	/*
	rc = _fat_block_get(&b, bs, idxp->devmap_handle, idxp->pfc, NULL,
	    (idxp->pdi * sizeof(fat_dentry_t)) / BPS(bs), BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		(void) fat_node_put(FS_NODE(nodep));
		return rc;
	}

	d = ((fat_dentry_t *)b->data) + (idxp->pdi % DPS(bs));
	*/

	nodep->lnkcnt = 1;
	nodep->refcnt = 1;

	rc = block_put(b);
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



/*
 * EXFAT libfs operations.
 */

int exfat_root_get(fs_node_t **rfn, devmap_handle_t devmap_handle)
{
	return exfat_node_get(rfn, devmap_handle, EXFAT_ROOT_IDX);
}

/*
int exfat_bitmap_get(fs_node_t **rfn, devmap_handle_t devmap_handle)
{
	return exfat_node_get(rfn, devmap_handle, EXFAT_BITMAP_IDX);
}
*/
/*
int exfat_uctable_get(fs_node_t **rfn, devmap_handle_t devmap_handle)
{
	return exfat_node_get(rfn, devmap_handle, EXFAT_UCTABLE_IDX);
}
*/

int exfat_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	*rfn = NULL;
	return EOK;
}

/** Instantiate a exFAT in-core node. */
int exfat_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle, fs_index_t index)
{
	exfat_node_t *nodep;
	exfat_idx_t *idxp;
	int rc;

	idxp = exfat_idx_get_by_index(devmap_handle, index);
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

int exfat_node_open(fs_node_t *fn)
{
	/*
	 * Opening a file is stateless, nothing
	 * to be done here.
	 */
	return EOK;
}

int exfat_node_put(fs_node_t *fn)
{
	return EOK;
}

int exfat_create_node(fs_node_t **rfn, devmap_handle_t devmap_handle, int flags)
{
	*rfn = NULL;
	return EOK;
}

int exfat_destroy_node(fs_node_t *fn)
{
	return EOK;
}

int exfat_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	return EOK;
}

int exfat_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	return EOK;
}

int exfat_has_children(bool *has_children, fs_node_t *fn)
{
	*has_children = false;
	return EOK;
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

char exfat_plb_get_char(unsigned pos)
{
	return exfat_reg.plb_ro[pos % PLB_SIZE];
}

bool exfat_is_directory(fs_node_t *fn)
{
	return EXFAT_NODE(fn)->type == EXFAT_DIRECTORY;
}

bool exfat_is_file(fs_node_t *fn)
{
	return EXFAT_NODE(fn)->type == EXFAT_FILE;
}

devmap_handle_t exfat_device_get(fs_node_t *node)
{
	return 0;
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
	.plb_get_char = exfat_plb_get_char,
	.is_directory = exfat_is_directory,
	.is_file = exfat_is_file,
	.device_get = exfat_device_get
};


/*
 * VFS operations.
 */

/* Print debug info */
static void exfat_fsinfo(exfat_bs_t *bs, devmap_handle_t devmap_handle)
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
	for (i=0; i<6; i++) {
		rc = fat_get_cluster(bs, devmap_handle, i, &clst);
		if (rc != EOK)
			return;
		printf("Clst %d: %x\n", i, clst);
	}
}

void exfat_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	exfat_node_t *rootp=NULL, *bitmapp=NULL, *uctablep=NULL;
	enum cache_mode cmode;
	exfat_bs_t *bs;

	/* Accept the mount options */
	char *opts;
	int rc = async_data_write_accept((void **) &opts, true, 0, 0, 0, NULL);

	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;

	free(opts);

	/* initialize libblock */
	rc = block_init(devmap_handle, BS_SIZE);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	/* prepare the boot block */
	rc = block_bb_read(devmap_handle, BS_BLOCK);
	if (rc != EOK) {
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	/* get the buffer with the boot sector */
	bs = block_bb_get(devmap_handle);

	/* Initialize the block cache */
	rc = block_cache_init(devmap_handle, BPS(bs), 0 /* XXX */, cmode);
	if (rc != EOK) {
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	/* Do some simple sanity checks on the file system. */
	rc = exfat_sanity_check(bs, devmap_handle);
	if (rc != EOK) {
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	rc = exfat_idx_init_by_devmap_handle(devmap_handle);
	if (rc != EOK) {
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	/* Initialize the root node. */
	rc = exfat_node_get_new_by_pos(&rootp, devmap_handle, EXFAT_ROOT_PAR, 
	    EXFAT_ROOT_POS);
	if (rc!=EOK) {
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOMEM);
		return;
	}
	assert(rootp->idx->index == EXFAT_ROOT_IDX);

	rootp->type = EXFAT_DIRECTORY;
	rootp->firstc = ROOT_FC(bs);
	rootp->fragmented = true;
	rootp->refcnt = 1;
	rootp->lnkcnt = 0;	/* FS root is not linked */

	uint32_t clusters;
	rc = fat_clusters_get(&clusters, bs, devmap_handle, rootp->firstc);
	if (rc != EOK) {
		free(rootp);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}
	rootp->size = BPS(bs) * SPC(bs) * clusters;
	fibril_mutex_unlock(&rootp->idx->lock);

	/* Open root directory and looking for Bitmap and UC-Table */
	exfat_directory_t di;
	exfat_dentry_t *de;
	rc = exfat_directory_open(rootp, &di);
	if (rc != EOK) {
		free(rootp);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}

	/* Initialize the bitmap node. */
	rc = exfat_directory_find(&di, EXFAT_DENTRY_BITMAP, &de);
	if (rc != EOK) {
		free(rootp);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}

	rc = exfat_node_get_new_by_pos(&bitmapp, devmap_handle, rootp->firstc, 
	    di.pos);
	if (rc!=EOK) {
		free(rootp);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOMEM);
		return;
	}
	assert(bitmapp->idx->index == EXFAT_BITMAP_IDX);
	fibril_mutex_unlock(&bitmapp->idx->lock);

	bitmapp->type = EXFAT_BITMAP;
	bitmapp->firstc = de->bitmap.firstc;
	bitmapp->fragmented = true;
	bitmapp->idx->parent_fragmented = true;
	bitmapp->refcnt = 1;
	bitmapp->lnkcnt = 0;
	bitmapp->size = de->bitmap.size;

	/* Initialize the uctable node. */
	rc = exfat_directory_seek(&di, 0);
	if (rc != EOK) {
		free(rootp);
		free(bitmapp);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}

	rc = exfat_directory_find(&di, EXFAT_DENTRY_UCTABLE, &de);
	if (rc != EOK) {
		free(rootp);
		free(bitmapp);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}

	rc = exfat_node_get_new_by_pos(&uctablep, devmap_handle, rootp->firstc, 
	    di.pos);
	if (rc!=EOK) {
		free(rootp);
		free(bitmapp);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOMEM);
		return;
	}
	assert(uctablep->idx->index == EXFAT_UCTABLE_IDX);
	fibril_mutex_unlock(&uctablep->idx->lock);

	uctablep->type = EXFAT_UCTABLE;
	uctablep->firstc = de->uctable.firstc;
	uctablep->fragmented = true;
	uctablep->idx->parent_fragmented = true;
	uctablep->refcnt = 1;
	uctablep->lnkcnt = 0;
	uctablep->size = de->uctable.size;

	rc = exfat_directory_close(&di);
	if (rc!=EOK) {
		free(rootp);
		free(bitmapp);
		free(uctablep);
		(void) block_cache_fini(devmap_handle);
		block_fini(devmap_handle);
		exfat_idx_fini_by_devmap_handle(devmap_handle);
		async_answer_0(rid, ENOMEM);
		return;
	}

	exfat_fsinfo(bs, devmap_handle);
	printf("Root dir size: %lld\n", rootp->size);

	async_answer_3(rid, EOK, rootp->idx->index, rootp->size, rootp->lnkcnt);
}

void exfat_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&exfat_libfs_ops, exfat_reg.fs_handle, rid, request);
}

void exfat_unmounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_node_t *fn;
	exfat_node_t *nodep;
	int rc;

	rc = exfat_root_get(&fn, devmap_handle);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}
	nodep = EXFAT_NODE(fn);

	/*
	 * We expect exactly two references on the root node. One for the
	 * fat_root_get() above and one created in fat_mounted().
	 */
	if (nodep->refcnt != 2) {
		(void) exfat_node_put(fn);
		async_answer_0(rid, EBUSY);
		return;
	}

	/*
	 * Put the root node and force it to the FAT free node list.
	 */
	(void) exfat_node_put(fn);
	(void) exfat_node_put(fn);

	/*
	 * Perform cleanup of the node structures, index structures and
	 * associated data. Write back this file system's dirty blocks and
	 * stop using libblock for this instance.
	 */
	(void) exfat_node_fini_by_devmap_handle(devmap_handle);
	exfat_idx_fini_by_devmap_handle(devmap_handle);
	(void) block_cache_fini(devmap_handle);
	block_fini(devmap_handle);

	async_answer_0(rid, EOK);
}

void exfat_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_unmount(&exfat_libfs_ops, rid, request);
}

/*
int bitmap_is_allocated(exfat_bs_t *bs, devmap_handle_t devmap_handle,
    exfat_cluster_t clst, bool *status)
{
	fs_node_t *fn;
	exfat_node_t *bitmap;
	int rc;

	rc = exfat_bitmap_get(&fn, devmap_handle);
	if (rc != EOK)
		return rc;

	nbitmap = EXFAT_NODE(fn);

	
	return EOK;
}
*/

/**
 * @}
 */
