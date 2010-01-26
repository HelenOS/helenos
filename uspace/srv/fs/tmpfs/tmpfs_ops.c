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
 * @file	tmpfs_ops.c
 * @brief	Implementation of VFS operations for the TMPFS file system
 *		server.
 */

#include "tmpfs.h"
#include "../../vfs/vfs.h"
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <atomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <adt/hash_table.h>
#include <as.h>
#include <libfs.h>

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

#define NODES_BUCKETS	256

/** All root nodes have index 0. */
#define TMPFS_SOME_ROOT		0
/** Global counter for assigning node indices. Shared by all instances. */
fs_index_t tmpfs_next_index = 1;

/*
 * Implementation of the libfs interface.
 */

/* Forward declarations of static functions. */
static int tmpfs_match(fs_node_t **, fs_node_t *, const char *);
static int tmpfs_node_get(fs_node_t **, dev_handle_t, fs_index_t);
static int tmpfs_node_open(fs_node_t *);
static int tmpfs_node_put(fs_node_t *);
static int tmpfs_create_node(fs_node_t **, dev_handle_t, int);
static int tmpfs_destroy_node(fs_node_t *);
static int tmpfs_link_node(fs_node_t *, fs_node_t *, const char *);
static int tmpfs_unlink_node(fs_node_t *, fs_node_t *, const char *);

/* Implementation of helper functions. */
static int tmpfs_root_get(fs_node_t **rfn, dev_handle_t dev_handle)
{
	return tmpfs_node_get(rfn, dev_handle, TMPFS_SOME_ROOT); 
}

static int tmpfs_has_children(bool *has_children, fs_node_t *fn)
{
	*has_children = !list_empty(&TMPFS_NODE(fn)->cs_head);
	return EOK;
}

static fs_index_t tmpfs_index_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->index;
}

static size_t tmpfs_size_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->size;
}

static unsigned tmpfs_lnkcnt_get(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->lnkcnt;
}

static char tmpfs_plb_get_char(unsigned pos)
{
	return tmpfs_reg.plb_ro[pos % PLB_SIZE];
}

static bool tmpfs_is_directory(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->type == TMPFS_DIRECTORY;
}

static bool tmpfs_is_file(fs_node_t *fn)
{
	return TMPFS_NODE(fn)->type == TMPFS_FILE;
}

static dev_handle_t tmpfs_device_get(fs_node_t *fn)
{
	return 0;
}

/** libfs operations */
libfs_ops_t tmpfs_libfs_ops = {
	.root_get = tmpfs_root_get,
	.match = tmpfs_match,
	.node_get = tmpfs_node_get,
	.node_open = tmpfs_node_open,
	.node_put = tmpfs_node_put,
	.create = tmpfs_create_node,
	.destroy = tmpfs_destroy_node,
	.link = tmpfs_link_node,
	.unlink = tmpfs_unlink_node,
	.has_children = tmpfs_has_children,
	.index_get = tmpfs_index_get,
	.size_get = tmpfs_size_get,
	.lnkcnt_get = tmpfs_lnkcnt_get,
	.plb_get_char = tmpfs_plb_get_char,
	.is_directory = tmpfs_is_directory,
	.is_file = tmpfs_is_file,
	.device_get = tmpfs_device_get
};

/** Hash table of all TMPFS nodes. */
hash_table_t nodes;

#define NODES_KEY_DEV	0	
#define NODES_KEY_INDEX	1

/* Implementation of hash table interface for the nodes hash table. */
static hash_index_t nodes_hash(unsigned long key[])
{
	return key[NODES_KEY_INDEX] % NODES_BUCKETS;
}

static int nodes_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	tmpfs_node_t *nodep = hash_table_get_instance(item, tmpfs_node_t,
	    nh_link);
	return (nodep->index == key[NODES_KEY_INDEX] &&
	    nodep->dev_handle == key[NODES_KEY_DEV]);
}

static void nodes_remove_callback(link_t *item)
{
}

/** TMPFS nodes hash table operations. */
hash_table_operations_t nodes_ops = {
	.hash = nodes_hash,
	.compare = nodes_compare,
	.remove_callback = nodes_remove_callback
};

static void tmpfs_node_initialize(tmpfs_node_t *nodep)
{
	nodep->bp = NULL;
	nodep->index = 0;
	nodep->dev_handle = 0;
	nodep->type = TMPFS_NONE;
	nodep->lnkcnt = 0;
	nodep->size = 0;
	nodep->data = NULL;
	link_initialize(&nodep->nh_link);
	list_initialize(&nodep->cs_head);
}

static void tmpfs_dentry_initialize(tmpfs_dentry_t *dentryp)
{
	link_initialize(&dentryp->link);
	dentryp->name = NULL;
	dentryp->node = NULL;
}

bool tmpfs_init(void)
{
	if (!hash_table_create(&nodes, NODES_BUCKETS, 2, &nodes_ops))
		return false;
	
	return true;
}

static bool tmpfs_instance_init(dev_handle_t dev_handle)
{
	fs_node_t *rfn;
	int rc;
	
	rc = tmpfs_create_node(&rfn, dev_handle, L_DIRECTORY);
	if (rc != EOK || !rfn)
		return false;
	TMPFS_NODE(rfn)->lnkcnt = 0;	/* FS root is not linked */
	return true;
}

int tmpfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	tmpfs_node_t *parentp = TMPFS_NODE(pfn);
	link_t *lnk;

	for (lnk = parentp->cs_head.next; lnk != &parentp->cs_head;
	    lnk = lnk->next) {
		tmpfs_dentry_t *dentryp;
		dentryp = list_get_instance(lnk, tmpfs_dentry_t, link);
		if (!str_cmp(dentryp->name, component)) {
			*rfn = FS_NODE(dentryp->node);
			return EOK;
		}
	}

	*rfn = NULL;
	return EOK;
}

int tmpfs_node_get(fs_node_t **rfn, dev_handle_t dev_handle, fs_index_t index)
{
	unsigned long key[] = {
		[NODES_KEY_DEV] = dev_handle,
		[NODES_KEY_INDEX] = index
	};
	link_t *lnk = hash_table_find(&nodes, key);
	if (lnk) {
		tmpfs_node_t *nodep;
		nodep = hash_table_get_instance(lnk, tmpfs_node_t, nh_link);
		*rfn = FS_NODE(nodep);
	} else {
		*rfn = NULL;
	}
	return EOK;	
}

int tmpfs_node_open(fs_node_t *fn)
{
	/* nothing to do */
	return EOK;
}

int tmpfs_node_put(fs_node_t *fn)
{
	/* nothing to do */
	return EOK;
}

int tmpfs_create_node(fs_node_t **rfn, dev_handle_t dev_handle, int lflag)
{
	fs_node_t *rootfn;
	int rc;

	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));

	tmpfs_node_t *nodep = malloc(sizeof(tmpfs_node_t));
	if (!nodep)
		return ENOMEM;
	tmpfs_node_initialize(nodep);
	nodep->bp = malloc(sizeof(fs_node_t));
	if (!nodep->bp) {
		free(nodep);
		return ENOMEM;
	}
	fs_node_initialize(nodep->bp);
	nodep->bp->data = nodep;	/* link the FS and TMPFS nodes */

	rc = tmpfs_root_get(&rootfn, dev_handle);
	assert(rc == EOK);
	if (!rootfn)
		nodep->index = TMPFS_SOME_ROOT;
	else
		nodep->index = tmpfs_next_index++;
	nodep->dev_handle = dev_handle;
	if (lflag & L_DIRECTORY) 
		nodep->type = TMPFS_DIRECTORY;
	else 
		nodep->type = TMPFS_FILE;

	/* Insert the new node into the nodes hash table. */
	unsigned long key[] = {
		[NODES_KEY_DEV] = nodep->dev_handle,
		[NODES_KEY_INDEX] = nodep->index
	};
	hash_table_insert(&nodes, key, &nodep->nh_link);
	*rfn = FS_NODE(nodep);
	return EOK;
}

int tmpfs_destroy_node(fs_node_t *fn)
{
	tmpfs_node_t *nodep = TMPFS_NODE(fn);
	
	assert(!nodep->lnkcnt);
	assert(list_empty(&nodep->cs_head));

	unsigned long key[] = {
		[NODES_KEY_DEV] = nodep->dev_handle,
		[NODES_KEY_INDEX] = nodep->index
	};
	hash_table_remove(&nodes, key, 2);

	if ((nodep->type == TMPFS_FILE) && (nodep->data))
		free(nodep->data);
	free(nodep->bp);
	free(nodep);
	return EOK;
}

int tmpfs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	tmpfs_node_t *parentp = TMPFS_NODE(pfn);
	tmpfs_node_t *childp = TMPFS_NODE(cfn);
	tmpfs_dentry_t *dentryp;
	link_t *lnk;

	assert(parentp->type == TMPFS_DIRECTORY);

	/* Check for duplicit entries. */
	for (lnk = parentp->cs_head.next; lnk != &parentp->cs_head;
	    lnk = lnk->next) {
		dentryp = list_get_instance(lnk, tmpfs_dentry_t, link);	
		if (!str_cmp(dentryp->name, nm))
			return EEXIST;
	}

	/* Allocate and initialize the dentry. */
	dentryp = malloc(sizeof(tmpfs_dentry_t));
	if (!dentryp)
		return ENOMEM;
	tmpfs_dentry_initialize(dentryp);

	/* Populate and link the new dentry. */
	size_t size = str_size(nm);
	dentryp->name = malloc(size + 1);
	if (!dentryp->name) {
		free(dentryp);
		return ENOMEM;
	}
	str_cpy(dentryp->name, size + 1, nm);
	dentryp->node = childp;
	childp->lnkcnt++;
	list_append(&dentryp->link, &parentp->cs_head);

	return EOK;
}

int tmpfs_unlink_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	tmpfs_node_t *parentp = TMPFS_NODE(pfn);
	tmpfs_node_t *childp = NULL;
	tmpfs_dentry_t *dentryp;
	link_t *lnk;

	if (!parentp)
		return EBUSY;
	
	for (lnk = parentp->cs_head.next; lnk != &parentp->cs_head;
	    lnk = lnk->next) {
		dentryp = list_get_instance(lnk, tmpfs_dentry_t, link);
		if (!str_cmp(dentryp->name, nm)) {
			childp = dentryp->node;
			assert(FS_NODE(childp) == cfn);
			break;
		}	
	}

	if (!childp)
		return ENOENT;
		
	if ((childp->lnkcnt == 1) && !list_empty(&childp->cs_head))
		return ENOTEMPTY;

	list_remove(&dentryp->link);
	free(dentryp);
	childp->lnkcnt--;

	return EOK;
}

void tmpfs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t) IPC_GET_ARG1(*request);
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

	/* Initialize TMPFS instance. */
	if (!tmpfs_instance_init(dev_handle)) {
		free(opts);
		ipc_answer_0(rid, ENOMEM);
		return;
	}

	fs_node_t *rootfn;
	rc = tmpfs_root_get(&rootfn, dev_handle);
	assert(rc == EOK);
	tmpfs_node_t *rootp = TMPFS_NODE(rootfn);
	if (str_cmp(opts, "restore") == 0) {
		if (tmpfs_restore(dev_handle))
			ipc_answer_3(rid, EOK, rootp->index, rootp->size,
			    rootp->lnkcnt);
		else
			ipc_answer_0(rid, ELIMIT);
	} else {
		ipc_answer_3(rid, EOK, rootp->index, rootp->size,
		    rootp->lnkcnt);
	}
	free(opts);
}

void tmpfs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&tmpfs_libfs_ops, tmpfs_reg.fs_handle, rid, request);
}

void tmpfs_unmounted(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, ENOTSUP);
}

void tmpfs_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_unmount(&tmpfs_libfs_ops, rid, request);
}

void tmpfs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&tmpfs_libfs_ops, tmpfs_reg.fs_handle, rid, request);
}

void tmpfs_read(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	off_t pos = (off_t)IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective TMPFS node.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[NODES_KEY_DEV] = dev_handle,
		[NODES_KEY_INDEX] = index
	};
	hlp = hash_table_find(&nodes, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_node_t *nodep = hash_table_get_instance(hlp, tmpfs_node_t,
	    nh_link);

	/*
	 * Receive the read request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);	
		ipc_answer_0(rid, EINVAL);
		return;
	}

	size_t bytes;
	if (nodep->type == TMPFS_FILE) {
		bytes = max(0, min(nodep->size - pos, size));
		(void) async_data_read_finalize(callid, nodep->data + pos,
		    bytes);
	} else {
		tmpfs_dentry_t *dentryp;
		link_t *lnk;
		int i;
		
		assert(nodep->type == TMPFS_DIRECTORY);
		
		/*
		 * Yes, we really use O(n) algorithm here.
		 * If it bothers someone, it could be fixed by introducing a
		 * hash table.
		 */
		for (i = 0, lnk = nodep->cs_head.next;
		    i < pos && lnk != &nodep->cs_head;
		    i++, lnk = lnk->next)
			;

		if (lnk == &nodep->cs_head) {
			ipc_answer_0(callid, ENOENT);
			ipc_answer_1(rid, ENOENT, 0);
			return;
		}

		dentryp = list_get_instance(lnk, tmpfs_dentry_t, link);

		(void) async_data_read_finalize(callid, dentryp->name,
		    str_size(dentryp->name) + 1);
		bytes = 1;
	}

	/*
	 * Answer the VFS_READ call.
	 */
	ipc_answer_1(rid, EOK, bytes);
}

void tmpfs_write(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	off_t pos = (off_t)IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective TMPFS node.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[NODES_KEY_DEV] = dev_handle,
		[NODES_KEY_INDEX] = index
	};
	hlp = hash_table_find(&nodes, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_node_t *nodep = hash_table_get_instance(hlp, tmpfs_node_t,
	    nh_link);

	/*
	 * Receive the write request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);	
		ipc_answer_0(rid, EINVAL);
		return;
	}

	/*
	 * Check whether the file needs to grow.
	 */
	if (pos + size <= nodep->size) {
		/* The file size is not changing. */
		(void) async_data_write_finalize(callid, nodep->data + pos, size);
		ipc_answer_2(rid, EOK, size, nodep->size);
		return;
	}
	size_t delta = (pos + size) - nodep->size;
	/*
	 * At this point, we are deliberately extremely straightforward and
	 * simply realloc the contents of the file on every write that grows the
	 * file. In the end, the situation might not be as bad as it may look:
	 * our heap allocator can save us and just grow the block whenever
	 * possible.
	 */
	void *newdata = realloc(nodep->data, nodep->size + delta);
	if (!newdata) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_2(rid, EOK, 0, nodep->size);
		return;
	}
	/* Clear any newly allocated memory in order to emulate gaps. */
	memset(newdata + nodep->size, 0, delta);
	nodep->size += delta;
	nodep->data = newdata;
	(void) async_data_write_finalize(callid, nodep->data + pos, size);
	ipc_answer_2(rid, EOK, size, nodep->size);
}

void tmpfs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	size_t size = (off_t)IPC_GET_ARG3(*request);

	/*
	 * Lookup the respective TMPFS node.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[NODES_KEY_DEV] = dev_handle,
		[NODES_KEY_INDEX] = index
	};
	hlp = hash_table_find(&nodes, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_node_t *nodep = hash_table_get_instance(hlp, tmpfs_node_t,
	    nh_link);

	if (size == nodep->size) {
		ipc_answer_0(rid, EOK);
		return;
	}

	void *newdata = realloc(nodep->data, size);
	if (!newdata) {
		ipc_answer_0(rid, ENOMEM);
		return;
	}
	if (size > nodep->size) {
		size_t delta = size - nodep->size;
		memset(newdata + nodep->size, 0, delta);
	}
	nodep->size = size;
	nodep->data = newdata;
	ipc_answer_0(rid, EOK);
}

void tmpfs_close(ipc_callid_t rid, ipc_call_t *request)
{
	ipc_answer_0(rid, EOK);
}

void tmpfs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	dev_handle_t dev_handle = (dev_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	int rc;

	link_t *hlp;
	unsigned long key[] = {
		[NODES_KEY_DEV] = dev_handle,
		[NODES_KEY_INDEX] = index
	};
	hlp = hash_table_find(&nodes, key);
	if (!hlp) {
		ipc_answer_0(rid, ENOENT);
		return;
	}
	tmpfs_node_t *nodep = hash_table_get_instance(hlp, tmpfs_node_t,
	    nh_link);
	rc = tmpfs_destroy_node(FS_NODE(nodep));
	ipc_answer_0(rid, rc);
}

void tmpfs_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_open_node(&tmpfs_libfs_ops, tmpfs_reg.fs_handle, rid, request);
}

void tmpfs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_stat(&tmpfs_libfs_ops, tmpfs_reg.fs_handle, rid, request);
}

void tmpfs_sync(ipc_callid_t rid, ipc_call_t *request)
{
	/* Dummy implementation */
	ipc_answer_0(rid, EOK);
}

/**
 * @}
 */
