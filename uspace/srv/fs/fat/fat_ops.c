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
#include "../../vfs/vfs.h"
#include <libfs.h>
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <string.h>
#include <byteorder.h>
#include <libadt/hash_table.h>
#include <libadt/list.h>
#include <assert.h>

#define BS_BLOCK		0

#define FIN_KEY_DEV_HANDLE	0
#define FIN_KEY_INDEX		1

/** Hash table of FAT in-core nodes. */
hash_table_t fin_hash;

/** List of free FAT in-core nodes. */
link_t ffn_head;

#define FAT_NAME_LEN		8
#define FAT_EXT_LEN		3

#define FAT_PAD			' ' 

#define FAT_DENTRY_UNUSED	0x00
#define FAT_DENTRY_E5_ESC	0x05
#define FAT_DENTRY_DOT		0x2e
#define FAT_DENTRY_ERASED	0xe5

static void dentry_name_canonify(fat_dentry_t *d, char *buf)
{
	int i;

	for (i = 0; i < FAT_NAME_LEN; i++) {
		if (d->name[i] == FAT_PAD) {
			buf++;
			break;
		}
		if (d->name[i] == FAT_DENTRY_E5_ESC)
			*buf++ = 0xe5;
		else
			*buf++ = d->name[i];
	}
	if (d->ext[0] != FAT_PAD)
		*buf++ = '.';
	for (i = 0; i < FAT_EXT_LEN; i++) {
		if (d->ext[i] == FAT_PAD) {
			*buf = '\0';
			return;
		}
		if (d->ext[i] == FAT_DENTRY_E5_ESC)
			*buf++ = 0xe5;
		else
			*buf++ = d->ext[i];
	}
}

/* TODO and also move somewhere else */
typedef struct {
	void *data;
} block_t;

static block_t *block_get(dev_handle_t dev_handle, off_t offset)
{
	return NULL;	/* TODO */
}

static block_t *fat_block_get(dev_handle_t dev_handle, fs_index_t index,
    off_t offset) {
	return NULL;	/* TODO */
}

static void block_put(block_t *block)
{
	/* TODO */
}

static void fat_node_initialize(fat_node_t *node)
{
	node->type = 0;
	node->index = 0;
	node->pindex = 0;
	node->dev_handle = 0;
	link_initialize(&node->fin_link);
	link_initialize(&node->ffn_link);
	node->size = 0;
	node->lnkcnt = 0;
	node->refcnt = 0;
	node->dirty = false;
}

static uint16_t fat_bps_get(dev_handle_t dev_handle)
{
	block_t *bb;
	uint16_t bps;
	
	bb = block_get(dev_handle, BS_BLOCK);
	assert(bb != NULL);
	bps = uint16_t_le2host(((fat_bs_t *)bb->data)->bps);
	block_put(bb);

	return bps;
}

typedef enum {
	FAT_DENTRY_SKIP,
	FAT_DENTRY_LAST,
	FAT_DENTRY_VALID
} fat_dentry_clsf_t;

static fat_dentry_clsf_t fat_classify_dentry(fat_dentry_t *d)
{
	if (d->attr & FAT_ATTR_VOLLABEL) {
		/* volume label entry */
		return FAT_DENTRY_SKIP;
	}
	if (d->name[0] == FAT_DENTRY_ERASED) {
		/* not-currently-used entry */
		return FAT_DENTRY_SKIP;
	}
	if (d->name[0] == FAT_DENTRY_UNUSED) {
		/* never used entry */
		return FAT_DENTRY_LAST;
	}
	if (d->name[0] == FAT_DENTRY_DOT) {
		/*
		 * Most likely '.' or '..'.
		 * It cannot occur in a regular file name.
		 */
		return FAT_DENTRY_SKIP;
	}
	return FAT_DENTRY_VALID;
}

static void fat_sync_node(fat_node_t *node)
{
	/* TODO */
}

/** Instantiate a FAT in-core node.
 *
 * FAT stores the info necessary for instantiation of a node in the parent of
 * that node.  This design necessitated the addition of the parent node index
 * parameter to this otherwise generic libfs API.
 */
static void *
fat_node_get(dev_handle_t dev_handle, fs_index_t index, fs_index_t pindex)
{
	link_t *lnk;
	fat_node_t *node = NULL;
	block_t *b;
	unsigned bps;
	unsigned dps;
	fat_dentry_t *d;
	unsigned i, j;

	unsigned long key[] = {
		[FIN_KEY_DEV_HANDLE] = dev_handle,
		[FIN_KEY_INDEX] = index
	};

	lnk = hash_table_find(&fin_hash, key);
	if (lnk) {
		/*
		 * The in-core node was found in the hash table.
		 */
		node = hash_table_get_instance(lnk, fat_node_t, fin_link);
		if (!node->refcnt++)
			list_remove(&node->ffn_link);
		return (void *) node;	
	}

	bps = fat_bps_get(dev_handle);
	dps = bps / sizeof(fat_dentry_t);
	
	if (!list_empty(&ffn_head)) {
		/*
		 * We are going to reuse a node from the free list.
		 */
		lnk = ffn_head.next; 
		list_remove(lnk);
		node = list_get_instance(lnk, fat_node_t, ffn_link);
		assert(!node->refcnt);
		if (node->dirty)
			fat_sync_node(node);
		key[FIN_KEY_DEV_HANDLE] = node->dev_handle;
		key[FIN_KEY_INDEX] = node->index;
		hash_table_remove(&fin_hash, key, sizeof(key)/sizeof(*key));
	} else {
		/*
		 * We need to allocate a new node.
		 */
		node = malloc(sizeof(fat_node_t));
		if (!node)
			return NULL;
	}
	fat_node_initialize(node);
	node->refcnt++;
	node->lnkcnt++;
	node->dev_handle = dev_handle;
	node->index = index;
	node->pindex = pindex;

	/*
	 * Because of the design of the FAT file system, we have no clue about
	 * how big (i.e. how many directory entries it contains) is the parent
	 * of the node we are trying to instantiate.  However, we know that it
	 * must contain a directory entry for our node of interest.  We simply
	 * scan the parent until we find it.
	 */
	for (i = 0; ; i++) {
		b = fat_block_get(node->dev_handle, node->pindex, i);
		for (j = 0; j < dps; j++) {
			d = ((fat_dentry_t *)b->data) + j;
			if (d->firstc == node->index)
				goto found;
		}
		block_put(b);
	}
	
found:
	if (!(d->attr & (FAT_ATTR_SUBDIR | FAT_ATTR_VOLLABEL)))
		node->type = FAT_FILE;
	if ((d->attr & FAT_ATTR_SUBDIR) || !index)
		node->type = FAT_DIRECTORY;
	assert((node->type == FAT_FILE) || (node->type == FAT_DIRECTORY));
	
	node->size = uint32_t_le2host(d->size);
	block_put(b);
	
	key[FIN_KEY_DEV_HANDLE] = node->dev_handle;
	key[FIN_KEY_INDEX] = node->index;
	hash_table_insert(&fin_hash, key, &node->fin_link);

	return node;
}

static void fat_node_put(void *node)
{
	fat_node_t *nodep = (fat_node_t *)node;

	if (nodep->refcnt-- == 1)
		list_append(&nodep->ffn_link, &ffn_head);
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

	bps = fat_bps_get(parentp->dev_handle);
	dps = bps / sizeof(fat_dentry_t);
	blocks = parentp->size / bps + (parentp->size % bps != 0);
	for (i = 0; i < blocks; i++) {
		unsigned dentries;
		
		b = fat_block_get(parentp->dev_handle, parentp->index, i);
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
				return NULL;
			default:
			case FAT_DENTRY_VALID:
				dentry_name_canonify(d, name);
				break;
			}
			if (strcmp(name, component) == 0) {
				/* hit */
				void *node = fat_node_get(parentp->dev_handle,
				    (fs_index_t)uint16_t_le2host(d->firstc),
				    parentp->index);
				block_put(b);
				return node;
			}
		}
		block_put(b);
	}

	return NULL;
}

static fs_index_t fat_index_get(void *node)
{
	fat_node_t *fnodep = (fat_node_t *)node;
	if (!fnodep)
		return 0;
	return fnodep->index;
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

	bps = fat_bps_get(nodep->dev_handle);
	dps = bps / sizeof(fat_dentry_t);

	blocks = nodep->size / bps + (nodep->size % bps != 0);

	for (i = 0; i < blocks; i++) {
		unsigned dentries;
		fat_dentry_t *d;
	
		b = fat_block_get(nodep->dev_handle, nodep->index, i);
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
				return false;
			default:
			case FAT_DENTRY_VALID:
				block_put(b);
				return true;
			}
			block_put(b);
			return true;
		}
		block_put(b);
	}

	return false;
}

static void *fat_root_get(dev_handle_t dev_handle)
{
	return fat_node_get(dev_handle, 0, 0);	
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
	.create = NULL,
	.destroy = NULL,
	.link = NULL,
	.unlink = NULL,
	.index_get = fat_index_get,
	.size_get = fat_size_get,
	.lnkcnt_get = fat_lnkcnt_get,
	.has_children = fat_has_children,
	.root_get = fat_root_get,
	.plb_get_char =	fat_plb_get_char,
	.is_directory = fat_is_directory,
	.is_file = fat_is_file
};

void fat_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&fat_libfs_ops, fat_reg.fs_handle, rid, request);
}

/**
 * @}
 */ 
