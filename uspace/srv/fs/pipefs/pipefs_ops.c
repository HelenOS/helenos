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
 * @file	pipefs_ops.c
 * @brief	Implementation of VFS operations for the PIPEFS file system
 *		server.
 */

#include "pipefs.h"
#include "../../vfs/vfs.h"
#include <macros.h>
#include <stdint.h>
#include <async.h>
#include <errno.h>
#include <atomic.h>
#include <stdlib.h>
#include <str.h>
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
#define PIPEFS_SOME_ROOT		0
/** Global counter for assigning node indices. Shared by all instances. */
fs_index_t pipefs_next_index = 1;

/*
 * Implementation of the libfs interface.
 */

/* Forward declarations of static functions. */
static int pipefs_match(fs_node_t **, fs_node_t *, const char *);
static int pipefs_node_get(fs_node_t **, devmap_handle_t, fs_index_t);
static int pipefs_node_open(fs_node_t *);
static int pipefs_node_put(fs_node_t *);
static int pipefs_create_node(fs_node_t **, devmap_handle_t, int);
static int pipefs_destroy_node(fs_node_t *);
static int pipefs_link_node(fs_node_t *, fs_node_t *, const char *);
static int pipefs_unlink_node(fs_node_t *, fs_node_t *, const char *);

/* Implementation of helper functions. */
static int pipefs_root_get(fs_node_t **rfn, devmap_handle_t devmap_handle)
{
	return pipefs_node_get(rfn, devmap_handle, PIPEFS_SOME_ROOT); 
}

static int pipefs_has_children(bool *has_children, fs_node_t *fn)
{
	*has_children = !list_empty(&PIPEFS_NODE(fn)->cs_head);
	return EOK;
}

static fs_index_t pipefs_index_get(fs_node_t *fn)
{
	return PIPEFS_NODE(fn)->index;
}

static aoff64_t pipefs_size_get(fs_node_t *fn)
{
	return 0;
}

static unsigned pipefs_lnkcnt_get(fs_node_t *fn)
{
	return PIPEFS_NODE(fn)->lnkcnt;
}

static char pipefs_plb_get_char(unsigned pos)
{
	return pipefs_reg.plb_ro[pos % PLB_SIZE];
}

static bool pipefs_is_directory(fs_node_t *fn)
{
	return PIPEFS_NODE(fn)->type == PIPEFS_DIRECTORY;
}

static bool pipefs_is_file(fs_node_t *fn)
{
	return PIPEFS_NODE(fn)->type == PIPEFS_FILE;
}

static devmap_handle_t pipefs_device_get(fs_node_t *fn)
{
	return 0;
}

/** libfs operations */
libfs_ops_t pipefs_libfs_ops = {
	.root_get = pipefs_root_get,
	.match = pipefs_match,
	.node_get = pipefs_node_get,
	.node_open = pipefs_node_open,
	.node_put = pipefs_node_put,
	.create = pipefs_create_node,
	.destroy = pipefs_destroy_node,
	.link = pipefs_link_node,
	.unlink = pipefs_unlink_node,
	.has_children = pipefs_has_children,
	.index_get = pipefs_index_get,
	.size_get = pipefs_size_get,
	.lnkcnt_get = pipefs_lnkcnt_get,
	.plb_get_char = pipefs_plb_get_char,
	.is_directory = pipefs_is_directory,
	.is_file = pipefs_is_file,
	.device_get = pipefs_device_get
};

/** Hash table of all PIPEFS nodes. */
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
	pipefs_node_t *nodep = hash_table_get_instance(item, pipefs_node_t,
	    nh_link);
	
	switch (keys) {
	case 1:
		return (nodep->devmap_handle == key[NODES_KEY_DEV]);
	case 2:	
		return ((nodep->devmap_handle == key[NODES_KEY_DEV]) &&
		    (nodep->index == key[NODES_KEY_INDEX]));
	default:
		assert((keys == 1) || (keys == 2));
	}

	return 0;
}

static void nodes_remove_callback(link_t *item)
{
	pipefs_node_t *nodep = hash_table_get_instance(item, pipefs_node_t,
	    nh_link);

	while (!list_empty(&nodep->cs_head)) {
		pipefs_dentry_t *dentryp = list_get_instance(nodep->cs_head.next,
		    pipefs_dentry_t, link);

		assert(nodep->type == PIPEFS_DIRECTORY);
		list_remove(&dentryp->link);
		free(dentryp);
	}

	free(nodep->bp);
	free(nodep);
}

/** PIPEFS nodes hash table operations. */
hash_table_operations_t nodes_ops = {
	.hash = nodes_hash,
	.compare = nodes_compare,
	.remove_callback = nodes_remove_callback
};

static void pipefs_node_initialize(pipefs_node_t *nodep)
{
	nodep->bp = NULL;
	nodep->index = 0;
	nodep->devmap_handle = 0;
	nodep->type = PIPEFS_NONE;
	nodep->lnkcnt = 0;
	nodep->start = 0;
	nodep->data = NULL;
	nodep->data_size = 0;
	fibril_mutex_initialize(&nodep->data_lock);
	fibril_condvar_initialize(&nodep->data_available);
	fibril_condvar_initialize(&nodep->data_consumed);
	link_initialize(&nodep->nh_link);
	list_initialize(&nodep->cs_head);
}

static void pipefs_dentry_initialize(pipefs_dentry_t *dentryp)
{
	link_initialize(&dentryp->link);
	dentryp->name = NULL;
	dentryp->node = NULL;
}

bool pipefs_init(void)
{
	if (!hash_table_create(&nodes, NODES_BUCKETS, 2, &nodes_ops))
		return false;
	
	return true;
}

static bool pipefs_instance_init(devmap_handle_t devmap_handle)
{
	fs_node_t *rfn;
	int rc;
	
	rc = pipefs_create_node(&rfn, devmap_handle, L_DIRECTORY);
	if (rc != EOK || !rfn)
		return false;
	PIPEFS_NODE(rfn)->lnkcnt = 0;	/* FS root is not linked */
	return true;
}

static void pipefs_instance_done(devmap_handle_t devmap_handle)
{
	unsigned long key[] = {
		[NODES_KEY_DEV] = devmap_handle
	};
	/*
	 * Here we are making use of one special feature of our hash table
	 * implementation, which allows to remove more items based on a partial
	 * key match. In the following, we are going to remove all nodes
	 * matching our device handle. The nodes_remove_callback() function will
	 * take care of resource deallocation.
	 */
	hash_table_remove(&nodes, key, 1);
}

int pipefs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	pipefs_node_t *parentp = PIPEFS_NODE(pfn);
	link_t *lnk;

	for (lnk = parentp->cs_head.next; lnk != &parentp->cs_head;
	    lnk = lnk->next) {
		pipefs_dentry_t *dentryp;
		dentryp = list_get_instance(lnk, pipefs_dentry_t, link);
		if (!str_cmp(dentryp->name, component)) {
			*rfn = FS_NODE(dentryp->node);
			return EOK;
		}
	}

	*rfn = NULL;
	return EOK;
}

int pipefs_node_get(fs_node_t **rfn, devmap_handle_t devmap_handle, fs_index_t index)
{
	unsigned long key[] = {
		[NODES_KEY_DEV] = devmap_handle,
		[NODES_KEY_INDEX] = index
	};
	link_t *lnk = hash_table_find(&nodes, key);
	if (lnk) {
		pipefs_node_t *nodep;
		nodep = hash_table_get_instance(lnk, pipefs_node_t, nh_link);
		*rfn = FS_NODE(nodep);
	} else {
		*rfn = NULL;
	}
	return EOK;	
}

int pipefs_node_open(fs_node_t *fn)
{
	/* nothing to do */
	return EOK;
}

int pipefs_node_put(fs_node_t *fn)
{
	/* nothing to do */
	return EOK;
}

int pipefs_create_node(fs_node_t **rfn, devmap_handle_t devmap_handle, int lflag)
{
	fs_node_t *rootfn;
	int rc;

	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));

	pipefs_node_t *nodep = malloc(sizeof(pipefs_node_t));
	if (!nodep)
		return ENOMEM;
	pipefs_node_initialize(nodep);
	nodep->bp = malloc(sizeof(fs_node_t));
	if (!nodep->bp) {
		free(nodep);
		return ENOMEM;
	}
	fs_node_initialize(nodep->bp);
	nodep->bp->data = nodep;	/* link the FS and PIPEFS nodes */

	rc = pipefs_root_get(&rootfn, devmap_handle);
	assert(rc == EOK);
	if (!rootfn)
		nodep->index = PIPEFS_SOME_ROOT;
	else
		nodep->index = pipefs_next_index++;
	nodep->devmap_handle = devmap_handle;
	if (lflag & L_DIRECTORY) 
		nodep->type = PIPEFS_DIRECTORY;
	else 
		nodep->type = PIPEFS_FILE;

	/* Insert the new node into the nodes hash table. */
	unsigned long key[] = {
		[NODES_KEY_DEV] = nodep->devmap_handle,
		[NODES_KEY_INDEX] = nodep->index
	};
	hash_table_insert(&nodes, key, &nodep->nh_link);
	*rfn = FS_NODE(nodep);
	return EOK;
}

int pipefs_destroy_node(fs_node_t *fn)
{
	pipefs_node_t *nodep = PIPEFS_NODE(fn);
	
	assert(!nodep->lnkcnt);
	assert(list_empty(&nodep->cs_head));

	unsigned long key[] = {
		[NODES_KEY_DEV] = nodep->devmap_handle,
		[NODES_KEY_INDEX] = nodep->index
	};
	hash_table_remove(&nodes, key, 2);

	/*
	 * The nodes_remove_callback() function takes care of the actual
	 * resource deallocation.
	 */
	return EOK;
}

int pipefs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	pipefs_node_t *parentp = PIPEFS_NODE(pfn);
	pipefs_node_t *childp = PIPEFS_NODE(cfn);
	pipefs_dentry_t *dentryp;
	link_t *lnk;

	assert(parentp->type == PIPEFS_DIRECTORY);

	/* Check for duplicit entries. */
	for (lnk = parentp->cs_head.next; lnk != &parentp->cs_head;
	    lnk = lnk->next) {
		dentryp = list_get_instance(lnk, pipefs_dentry_t, link);	
		if (!str_cmp(dentryp->name, nm))
			return EEXIST;
	}

	/* Allocate and initialize the dentry. */
	dentryp = malloc(sizeof(pipefs_dentry_t));
	if (!dentryp)
		return ENOMEM;
	pipefs_dentry_initialize(dentryp);

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

int pipefs_unlink_node(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	pipefs_node_t *parentp = PIPEFS_NODE(pfn);
	pipefs_node_t *childp = NULL;
	pipefs_dentry_t *dentryp;
	link_t *lnk;

	if (!parentp)
		return EBUSY;
	
	for (lnk = parentp->cs_head.next; lnk != &parentp->cs_head;
	    lnk = lnk->next) {
		dentryp = list_get_instance(lnk, pipefs_dentry_t, link);
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

void pipefs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_node_t *rootfn;
	int rc;
	
	/* Accept the mount options. */
	char *opts;
	rc = async_data_write_accept((void **) &opts, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	/* Check if this device is not already mounted. */
	rc = pipefs_root_get(&rootfn, devmap_handle);
	if ((rc == EOK) && (rootfn)) {
		(void) pipefs_node_put(rootfn);
		free(opts);
		async_answer_0(rid, EEXIST);
		return;
	}

	/* Initialize PIPEFS instance. */
	if (!pipefs_instance_init(devmap_handle)) {
		free(opts);
		async_answer_0(rid, ENOMEM);
		return;
	}

	rc = pipefs_root_get(&rootfn, devmap_handle);
	assert(rc == EOK);
	pipefs_node_t *rootp = PIPEFS_NODE(rootfn);
	async_answer_3(rid, EOK, rootp->index, 0, rootp->lnkcnt);
	free(opts);
}

void pipefs_mount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_mount(&pipefs_libfs_ops, pipefs_reg.fs_handle, rid, request);
}

void pipefs_unmounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);

	pipefs_instance_done(devmap_handle);
	async_answer_0(rid, EOK);
}

void pipefs_unmount(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_unmount(&pipefs_libfs_ops, rid, request);
}

void pipefs_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_lookup(&pipefs_libfs_ops, pipefs_reg.fs_handle, rid, request);
}

void pipefs_read(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	aoff64_t pos =
	    (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request), IPC_GET_ARG4(*request));
	
	/*
	 * Lookup the respective PIPEFS node.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[NODES_KEY_DEV] = devmap_handle,
		[NODES_KEY_INDEX] = index
	};
	hlp = hash_table_find(&nodes, key);
	if (!hlp) {
		async_answer_0(rid, ENOENT);
		return;
	}
	pipefs_node_t *nodep = hash_table_get_instance(hlp, pipefs_node_t,
	    nh_link);
	
	/*
	 * Receive the read request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(rid, EINVAL);
		return;
	}

	size_t bytes;
	if (nodep->type == PIPEFS_FILE) {
		fibril_mutex_lock(&nodep->data_lock);
		
		/*
		 * Check if the client didn't seek somewhere else
		 */
		if (pos != nodep->start) {
			async_answer_0(callid, ENOTSUP);
			async_answer_0(rid, ENOTSUP);
			fibril_mutex_unlock(&nodep->data_lock);
			return;
		}
		
		if (nodep->data == NULL || nodep->data_size > 0) {
			// Wait for the data
			fibril_condvar_wait(&nodep->data_available, &nodep->data_lock);
		}
		
		assert(nodep->data != NULL);
		assert(nodep->data_size > 0);
		
		bytes = min(size, nodep->data_size);
		
		(void) async_data_read_finalize(callid, nodep->data, bytes);
		
		nodep->data += bytes;
		nodep->data_size -= bytes;
		nodep->start += bytes;
		
		if (nodep->data_size == 0) {
			nodep->data = NULL;
			fibril_condvar_broadcast(&nodep->data_consumed);
		}
		
		fibril_mutex_unlock(&nodep->data_lock);
	} else {
		pipefs_dentry_t *dentryp;
		link_t *lnk;
		aoff64_t i;
		
		assert(nodep->type == PIPEFS_DIRECTORY);
		
		/*
		 * Yes, we really use O(n) algorithm here.
		 * If it bothers someone, it could be fixed by introducing a
		 * hash table.
		 */
		for (i = 0, lnk = nodep->cs_head.next;
		    (i < pos) && (lnk != &nodep->cs_head);
		    i++, lnk = lnk->next)
			;

		if (lnk == &nodep->cs_head) {
			async_answer_0(callid, ENOENT);
			async_answer_1(rid, ENOENT, 0);
			return;
		}

		dentryp = list_get_instance(lnk, pipefs_dentry_t, link);

		(void) async_data_read_finalize(callid, dentryp->name,
		    str_size(dentryp->name) + 1);
		bytes = 1;
	}

	/*
	 * Answer the VFS_READ call.
	 */
	async_answer_1(rid, EOK, bytes);
}

void pipefs_write(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t) IPC_GET_ARG2(*request);
	aoff64_t pos =
	    (aoff64_t) MERGE_LOUP32(IPC_GET_ARG3(*request), IPC_GET_ARG4(*request));
	
	/*
	 * Lookup the respective PIPEFS node.
	 */
	link_t *hlp;
	unsigned long key[] = {
		[NODES_KEY_DEV] = devmap_handle,
		[NODES_KEY_INDEX] = index
	};
	hlp = hash_table_find(&nodes, key);
	if (!hlp) {
		async_answer_0(rid, ENOENT);
		return;
	}
	pipefs_node_t *nodep = hash_table_get_instance(hlp, pipefs_node_t,
	    nh_link);

	/*
	 * Receive the write request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);	
		async_answer_0(rid, EINVAL);
		return;
	}
	
	if (size == 0) {
		async_data_write_finalize(callid, NULL, 0);
		async_answer_2(rid, EOK, 0, 0);
		return;
	}
	
	fibril_mutex_lock(&nodep->data_lock);

	/*
	 * Check whether we are writing to the end
	 */
	if (pos != nodep->start+nodep->data_size) {
		fibril_mutex_unlock(&nodep->data_lock);
		async_answer_0(callid, ENOTSUP);
		async_answer_0(rid, ENOTSUP);
		return;
	}
	
	/*
	 * Wait until there is no data buffer
	 */
	if (nodep->data != NULL) {
		fibril_condvar_wait(&nodep->data_consumed, &nodep->data_lock);
	}
	
	assert(nodep->data == NULL);
	
	/*
	 * Allocate a buffer for the new data.
	 * Currently we accept any size
	 */
	void *newdata = malloc(size);
	if (!newdata) {
		fibril_mutex_unlock(&nodep->data_lock);
		async_answer_0(callid, ENOMEM);
		async_answer_0(rid, ENOMEM);
		return;
	}
	
	(void) async_data_write_finalize(callid, newdata, size);
	
	nodep->data = newdata;
	nodep->data_size = size;
	
	fibril_mutex_unlock(&nodep->data_lock);
	
	// Signal that the data is ready
	fibril_condvar_signal(&nodep->data_available);
	
	fibril_mutex_lock(&nodep->data_lock);
	
	// Wait until all data is consumed
	fibril_condvar_wait(&nodep->data_consumed, &nodep->data_lock);
	
	assert(nodep->data == NULL);
	
	fibril_mutex_unlock(&nodep->data_lock);
	free(newdata);
	
	async_answer_2(rid, EOK, size, 0);
}

void pipefs_truncate(ipc_callid_t rid, ipc_call_t *request)
{
	/*
	 * PIPEFS does not support resizing of files
	 */
	async_answer_0(rid, ENOTSUP);
}

void pipefs_close(ipc_callid_t rid, ipc_call_t *request)
{
	async_answer_0(rid, EOK);
}

void pipefs_destroy(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t)IPC_GET_ARG1(*request);
	fs_index_t index = (fs_index_t)IPC_GET_ARG2(*request);
	int rc;

	link_t *hlp;
	unsigned long key[] = {
		[NODES_KEY_DEV] = devmap_handle,
		[NODES_KEY_INDEX] = index
	};
	hlp = hash_table_find(&nodes, key);
	if (!hlp) {
		async_answer_0(rid, ENOENT);
		return;
	}
	pipefs_node_t *nodep = hash_table_get_instance(hlp, pipefs_node_t,
	    nh_link);
	rc = pipefs_destroy_node(FS_NODE(nodep));
	async_answer_0(rid, rc);
}

void pipefs_open_node(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_open_node(&pipefs_libfs_ops, pipefs_reg.fs_handle, rid, request);
}

void pipefs_stat(ipc_callid_t rid, ipc_call_t *request)
{
	libfs_stat(&pipefs_libfs_ops, pipefs_reg.fs_handle, rid, request);
}

void pipefs_sync(ipc_callid_t rid, ipc_call_t *request)
{
	/*
	 * PIPEFS keeps its data structures always consistent,
	 * thus the sync operation is a no-op.
	 */
	async_answer_0(rid, EOK);
}

/**
 * @}
 */
