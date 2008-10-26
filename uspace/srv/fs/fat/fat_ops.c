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

static int dev_phone = -1;		/* FIXME */
static void *dev_buffer = NULL;		/* FIXME */

block_t *block_get(dev_handle_t dev_handle, off_t offset, size_t bs)
{
	/* FIXME */
	block_t *b;
	off_t bufpos = 0;
	size_t buflen = 0;
	off_t pos = offset * bs;

	assert(dev_phone != -1);
	assert(dev_buffer);

	b = malloc(sizeof(block_t));
	if (!b)
		return NULL;
	
	b->data = malloc(bs);
	if (!b->data) {
		free(b);
		return NULL;
	}
	b->size = bs;

	if (!libfs_blockread(dev_phone, dev_buffer, &bufpos, &buflen, &pos,
	    b->data, bs, bs)) {
		free(b->data);
		free(b);
		return NULL;
	}

	return b;
}

void block_put(block_t *block)
{
	/* FIXME */
	free(block->data);
	free(block);
}

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
	/* TODO */
}

/** Internal version of fat_node_get().
 *
 * @param idxp		Locked index structure.
 */
static void *fat_node_get_core(fat_idx_t *idxp)
{
	block_t *b;
	fat_dentry_t *d;
	fat_node_t *nodep = NULL;
	unsigned bps;
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

	bps = fat_bps_get(idxp->dev_handle);
	dps = bps / sizeof(fat_dentry_t);

	/* Read the block that contains the dentry of interest. */
	b = _fat_block_get(idxp->dev_handle, idxp->pfc,
	    (idxp->pdi * sizeof(fat_dentry_t)) / bps);
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
		nodep->size = bps * _fat_blcks_get(idxp->dev_handle,
		    uint16_t_le2host(d->firstc), NULL);
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

/** Instantiate a FAT in-core node. */
static void *fat_node_get(dev_handle_t dev_handle, fs_index_t index)
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

static void fat_node_put(void *node)
{
	fat_node_t *nodep = (fat_node_t *)node;

	futex_down(&nodep->lock);
	if (!--nodep->refcnt) {
		futex_down(&ffn_futex);
		list_append(&nodep->ffn_link, &ffn_head);
		futex_up(&ffn_futex);
	}
	futex_up(&nodep->lock);
}

static void *fat_create(int flags)
{
	return NULL;	/* not supported at the moment */
}

static int fat_destroy(void *node)
{
	return ENOTSUP;	/* not supported at the moment */
}

static bool fat_link(void *prnt, void *chld, const char *name)
{
	return false;	/* not supported at the moment */
}

static int fat_unlink(void *prnt, void *chld)
{
	return ENOTSUP;	/* not supported at the moment */
}

static void *fat_match(void *prnt, const char *component)
{
	fat_node_t *parentp = (fat_node_t *)prnt;
	char name[FAT_NAME_LEN + 1 + FAT_EXT_LEN + 1];
	unsigned i, j;
	unsigned bps;		/* bytes per sector */
	unsigned dps;		/* dentries per sector */
	unsigned blocks;
	fat_dentry_t *d;
	block_t *b;

	futex_down(&parentp->idx->lock);
	bps = fat_bps_get(parentp->idx->dev_handle);
	dps = bps / sizeof(fat_dentry_t);
	blocks = parentp->size / bps + (parentp->size % bps != 0);
	for (i = 0; i < blocks; i++) {
		unsigned dentries;
		
		b = fat_block_get(parentp, i);
		dentries = (i == blocks - 1) ?
		    parentp->size % sizeof(fat_dentry_t) :
		    dps;
		for (j = 0; j < dentries; j++) { 
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
				continue;
			case FAT_DENTRY_LAST:
				block_put(b);
				futex_up(&parentp->idx->lock);
				return NULL;
			default:
			case FAT_DENTRY_VALID:
				dentry_name_canonify(d, name);
				break;
			}
			if (stricmp(name, component) == 0) {
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

static fs_index_t fat_index_get(void *node)
{
	fat_node_t *fnodep = (fat_node_t *)node;
	if (!fnodep)
		return 0;
	return fnodep->idx->index;
}

static size_t fat_size_get(void *node)
{
	return ((fat_node_t *)node)->size;
}

static unsigned fat_lnkcnt_get(void *node)
{
	return ((fat_node_t *)node)->lnkcnt;
}

static bool fat_has_children(void *node)
{
	fat_node_t *nodep = (fat_node_t *)node;
	unsigned bps;
	unsigned dps;
	unsigned blocks;
	block_t *b;
	unsigned i, j;

	if (nodep->type != FAT_DIRECTORY)
		return false;

	futex_down(&nodep->idx->lock);
	bps = fat_bps_get(nodep->idx->dev_handle);
	dps = bps / sizeof(fat_dentry_t);

	blocks = nodep->size / bps + (nodep->size % bps != 0);

	for (i = 0; i < blocks; i++) {
		unsigned dentries;
		fat_dentry_t *d;
	
		b = fat_block_get(nodep, i);
		dentries = (i == blocks - 1) ?
		    nodep->size % sizeof(fat_dentry_t) :
		    dps;
		for (j = 0; j < dentries; j++) {
			d = ((fat_dentry_t *)b->data) + j;
			switch (fat_classify_dentry(d)) {
			case FAT_DENTRY_SKIP:
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

static void *fat_root_get(dev_handle_t dev_handle)
{
	return fat_node_get(dev_handle, 0);
}

static char fat_plb_get_char(unsigned pos)
{
	return fat_reg.plb_ro[pos % PLB_SIZE];
}

static bool fat_is_directory(void *node)
{
	return ((fat_node_t *)node)->type == FAT_DIRECTORY;
}

static bool fat_is_file(void *node)
{
	return ((fat_node_t *)node)->type == FAT_FILE;
}

/** libfs operations */
libfs_ops_t fat_libfs_ops = {
	.match = fat_match,
	.node_get = fat_node_get,
	.node_put = fat_node_put,
	.create = fat_create,
	.destroy = fat_destroy,
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

void fat_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
	block_t *bb;
	uint16_t bps;
	uint16_t rde;
	int rc;

	/*
	 * For now, we don't bother to remember dev_handle, dev_phone or
	 * dev_buffer in some data structure. We use global variables because we
	 * know there will be at most one mount on this file system.
	 * Of course, this is a huge TODO item.
	 */
	dev_buffer = mmap(NULL, BS_SIZE, PROTO_READ | PROTO_WRITE,
	    MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	
	if (!dev_buffer) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	dev_phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP,
	    DEVMAP_CONNECT_TO_DEVICE, dev_handle);

	if (dev_phone < 0) {
		munmap(dev_buffer, BS_SIZE);
		ipc_answer_0(rid, dev_phone);
		return;
	}

	rc = ipc_share_out_start(dev_phone, dev_buffer,
	    AS_AREA_READ | AS_AREA_WRITE);
	if (rc != EOK) {
	    	munmap(dev_buffer, BS_SIZE);
		ipc_answer_0(rid, rc);
		return;
	}

	/* Read the number of root directory entries. */
	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	rde = uint16_t_le2host(FAT_BS(bb)->root_ent_max);
	block_put(bb);

	if (bps != BS_SIZE) {
		munmap(dev_buffer, BS_SIZE);
		ipc_answer_0(rid, ENOTSUP);
		return;
	}

	rc = fat_idx_init_by_dev_handle(dev_handle);
	if (rc != EOK) {
	    	munmap(dev_buffer, BS_SIZE);
		ipc_answer_0(rid, rc);
		return;
	}

	/* Initialize the root node. */
	fat_node_t *rootp = (fat_node_t *)malloc(sizeof(fat_node_t));
	if (!rootp) {
	    	munmap(dev_buffer, BS_SIZE);
		fat_idx_fini_by_dev_handle(dev_handle);
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	fat_node_initialize(rootp);

	fat_idx_t *ridxp = fat_idx_get_by_pos(dev_handle, FAT_CLST_ROOTPAR, 0);
	if (!ridxp) {
	    	munmap(dev_buffer, BS_SIZE);
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
	uint16_t bps = fat_bps_get(dev_handle);
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

	if (nodep->type == FAT_FILE) {
		/*
		 * Our strategy for regular file reads is to read one block at
		 * most and make use of the possibility to return less data than
		 * requested. This keeps the code very simple.
		 */
		bytes = min(len, bps - pos % bps);
		b = fat_block_get(nodep, pos / bps);
		(void) ipc_data_read_finalize(callid, b->data + pos % bps,
		    bytes);
		block_put(b);
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

			b = fat_block_get(nodep, bnum);
			for (o = pos % (bps / sizeof(fat_dentry_t));
			    o < bps / sizeof(fat_dentry_t);
			    o++, pos++) {
				d = ((fat_dentry_t *)b->data) + o;
				switch (fat_classify_dentry(d)) {
				case FAT_DENTRY_SKIP:
					continue;
				case FAT_DENTRY_LAST:
					block_put(b);
					goto miss;
				default:
				case FAT_DENTRY_VALID:
					dentry_name_canonify(d, name);
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
		(void) ipc_data_read_finalize(callid, name, strlen(name) + 1);
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
	size_t bytes;
	block_t *b, *bb;
	uint16_t bps;
	unsigned spc;
	off_t boundary;
	
	if (!nodep) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	
	/* XXX remove me when you are ready */
	{
		ipc_answer_0(rid, ENOTSUP);
		fat_node_put(nodep);
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

	/*
	 * In all scenarios, we will attempt to write out only one block worth
	 * of data at maximum. There might be some more efficient approaches,
	 * but this one greatly simplifies fat_write(). Note that we can afford
	 * to do this because the client must be ready to handle the return
	 * value signalizing a smaller number of bytes written. 
	 */ 
	bytes = min(len, bps - pos % bps);

	bb = block_get(dev_handle, BS_BLOCK, BS_SIZE);
	bps = uint16_t_le2host(FAT_BS(bb)->bps);
	spc = FAT_BS(bb)->spc;
	block_put(bb);
	
	boundary = ROUND_UP(nodep->size, bps * spc);
	if (pos < boundary) {
		/*
		 * This is the easier case - we are either overwriting already
		 * existing contents or writing behind the EOF, but still within
		 * the limits of the last cluster. The node size may grow to the
		 * next block size boundary.
		 */
		fat_fill_gap(nodep, FAT_CLST_RES0, pos);
		b = fat_block_get(nodep, pos / bps);
		(void) ipc_data_write_finalize(callid, b->data + pos % bps,
		    bytes);
		b->dirty = true;		/* need to sync block */
		block_put(b);
		if (pos + bytes > nodep->size) {
			nodep->size = pos + bytes;
			nodep->dirty = true;	/* need to sync node */
		}
		fat_node_put(nodep);
		ipc_answer_1(rid, EOK, bytes);	
		return;
	} else {
		/*
		 * This is the more difficult case. We must allocate new
		 * clusters for the node and zero them out.
		 */
		int status;
		unsigned nclsts;
		fat_cluster_t mcl, lcl;
	
		nclsts = (ROUND_UP(pos + bytes, bps * spc) - boundary) /
		    bps * spc;
		/* create an independent chain of nclsts clusters in all FATs */
		status = fat_alloc_clusters(dev_handle, nclsts, &mcl, &lcl);
		if (status != EOK) {
			/* could not allocate a chain of nclsts clusters */
			fat_node_put(nodep);
			ipc_answer_0(callid, status);
			ipc_answer_0(rid, status);
			return;
		}
		/* zero fill any gaps */
		fat_fill_gap(nodep, mcl, pos);
		b = _fat_block_get(dev_handle, lcl, (pos / bps) % spc);
		(void) ipc_data_write_finalize(callid, b->data + pos % bps,
		    bytes);
		b->dirty = true;		/* need to sync block */
		block_put(b);
		/*
		 * Append the cluster chain starting in mcl to the end of the
		 * node's cluster chain.
		 */
		fat_append_clusters(nodep, mcl);
		nodep->size = pos + bytes;
		nodep->dirty = true;		/* need to sync node */
		fat_node_put(nodep);
		ipc_answer_1(rid, EOK, bytes);
		return;
	}
}

/**
 * @}
 */ 
