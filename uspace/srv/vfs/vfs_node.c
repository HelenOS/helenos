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
 * @file	vfs_node.c
 * @brief	Various operations on VFS nodes have their home in this file.
 */

#include "vfs.h"
#include <stdlib.h>
#include <str.h>
#include <fibril_synch.h>
#include <adt/hash_table.h>
#include <assert.h>
#include <async.h>
#include <errno.h>

/** Mutex protecting the VFS node hash table. */
FIBRIL_MUTEX_INITIALIZE(nodes_mutex);

#define NODES_BUCKETS_LOG	8
#define NODES_BUCKETS		(1 << NODES_BUCKETS_LOG)

/** VFS node hash table containing all active, in-memory VFS nodes. */
hash_table_t nodes;

#define KEY_FS_HANDLE	0
#define KEY_DEV_HANDLE	1
#define KEY_INDEX	2

static hash_index_t nodes_hash(unsigned long []);
static int nodes_compare(unsigned long [], hash_count_t, link_t *);
static void nodes_remove_callback(link_t *);

/** VFS node hash table operations. */
hash_table_operations_t nodes_ops = {
	.hash = nodes_hash,
	.compare = nodes_compare,
	.remove_callback = nodes_remove_callback
};

/** Initialize the VFS node hash table.
 *
 * @return		Return true on success, false on failure.
 */
bool vfs_nodes_init(void)
{
	return hash_table_create(&nodes, NODES_BUCKETS, 3, &nodes_ops);
}

static inline void _vfs_node_addref(vfs_node_t *node)
{
	node->refcnt++;
}

/** Increment reference count of a VFS node.
 *
 * @param node		VFS node that will have its refcnt incremented.
 */
void vfs_node_addref(vfs_node_t *node)
{
	fibril_mutex_lock(&nodes_mutex);
	_vfs_node_addref(node);
	fibril_mutex_unlock(&nodes_mutex);
}

/** Decrement reference count of a VFS node.
 *
 * This function handles the case when the reference count drops to zero.
 *
 * @param node		VFS node that will have its refcnt decremented.
 */
void vfs_node_delref(vfs_node_t *node)
{
	bool free_vfs_node = false;
	bool free_fs_node = false;
	
	fibril_mutex_lock(&nodes_mutex);
	
	if (node->refcnt-- == 1) {
		
		/*
		 * We are dropping the last reference to this node.
		 * Remove it from the VFS node hash table.
		 */
		
		unsigned long key[] = {
			[KEY_FS_HANDLE] = node->fs_handle,
			[KEY_DEV_HANDLE] = node->service_id,
			[KEY_INDEX] = node->index
		};
		
		hash_table_remove(&nodes, key, 3);
		free_vfs_node = true;
		
		if (!node->lnkcnt)
			free_fs_node = true;
	}
	
	fibril_mutex_unlock(&nodes_mutex);
	
	if (free_fs_node) {
		
		/*
		 * The node is not visible in the file system namespace.
		 * Free up its resources.
		 */
		
		async_exch_t *exch = vfs_exchange_grab(node->fs_handle);
		sysarg_t rc = async_req_2_0(exch, VFS_OUT_DESTROY,
		    (sysarg_t) node->service_id, (sysarg_t)node->index);
		
		assert(rc == EOK);
		vfs_exchange_release(exch);
	}
	
	if (free_vfs_node)
		free(node);
}

/** Forget node.
 *
 * This function will remove the node from the node hash table and deallocate
 * its memory, regardless of the node's reference count.
 *
 * @param node	Node to be forgotten.
 */
void vfs_node_forget(vfs_node_t *node)
{
	fibril_mutex_lock(&nodes_mutex);
	unsigned long key[] = {
		[KEY_FS_HANDLE] = node->fs_handle,
		[KEY_DEV_HANDLE] = node->service_id,
		[KEY_INDEX] = node->index
	};
	hash_table_remove(&nodes, key, 3);
	fibril_mutex_unlock(&nodes_mutex);
	free(node);
}

/** Find VFS node.
 *
 * This function will try to lookup the given triplet in the VFS node hash
 * table. In case the triplet is not found there, a new VFS node is created.
 * In any case, the VFS node will have its reference count incremented. Every
 * node returned by this call should be eventually put back by calling
 * vfs_node_put() on it.
 *
 * @param result	Populated lookup result structure.
 *
 * @return		VFS node corresponding to the given triplet.
 */
vfs_node_t *vfs_node_get(vfs_lookup_res_t *result)
{
	unsigned long key[] = {
		[KEY_FS_HANDLE] = result->triplet.fs_handle,
		[KEY_DEV_HANDLE] = result->triplet.service_id,
		[KEY_INDEX] = result->triplet.index
	};
	link_t *tmp;
	vfs_node_t *node;

	fibril_mutex_lock(&nodes_mutex);
	tmp = hash_table_find(&nodes, key);
	if (!tmp) {
		node = (vfs_node_t *) malloc(sizeof(vfs_node_t));
		if (!node) {
			fibril_mutex_unlock(&nodes_mutex);
			return NULL;
		}
		memset(node, 0, sizeof(vfs_node_t));
		node->fs_handle = result->triplet.fs_handle;
		node->service_id = result->triplet.service_id;
		node->index = result->triplet.index;
		node->size = result->size;
		node->lnkcnt = result->lnkcnt;
		node->type = result->type;
		link_initialize(&node->nh_link);
		fibril_rwlock_initialize(&node->contents_rwlock);
		hash_table_insert(&nodes, key, &node->nh_link);
	} else {
		node = hash_table_get_instance(tmp, vfs_node_t, nh_link);
		if (node->type == VFS_NODE_UNKNOWN &&
		    result->type != VFS_NODE_UNKNOWN) {
			/* Upgrade the node type. */
			node->type = result->type;
		}
	}

	assert(node->size == result->size || node->type != VFS_NODE_FILE);
	assert(node->lnkcnt == result->lnkcnt);
	assert(node->type == result->type || result->type == VFS_NODE_UNKNOWN);

	_vfs_node_addref(node);
	fibril_mutex_unlock(&nodes_mutex);

	return node;
}

/** Return VFS node when no longer needed by the caller.
 *
 * This function will remove the reference on the VFS node created by
 * vfs_node_get(). This function can only be called as a closing bracket to the
 * preceding vfs_node_get() call.
 *
 * @param node		VFS node being released.
 */
void vfs_node_put(vfs_node_t *node)
{
	vfs_node_delref(node);
}

hash_index_t nodes_hash(unsigned long key[])
{
	hash_index_t a = key[KEY_FS_HANDLE] << (NODES_BUCKETS_LOG / 4);
	hash_index_t b = (a | key[KEY_DEV_HANDLE]) << (NODES_BUCKETS_LOG / 2);
	
	return (b | key[KEY_INDEX]) & (NODES_BUCKETS - 1);
}

int nodes_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	vfs_node_t *node = hash_table_get_instance(item, vfs_node_t, nh_link);
	return (node->fs_handle == (fs_handle_t) key[KEY_FS_HANDLE]) &&
	    (node->service_id == key[KEY_DEV_HANDLE]) &&
	    (node->index == key[KEY_INDEX]);
}

void nodes_remove_callback(link_t *item)
{
}

struct refcnt_data {
	/** Sum of all reference counts for this file system instance. */
	unsigned refcnt;
	fs_handle_t fs_handle;
	service_id_t service_id;
};

static void refcnt_visitor(link_t *item, void *arg)
{
	vfs_node_t *node = hash_table_get_instance(item, vfs_node_t, nh_link);
	struct refcnt_data *rd = (void *) arg;

	if ((node->fs_handle == rd->fs_handle) &&
	    (node->service_id == rd->service_id))
		rd->refcnt += node->refcnt;
}

unsigned
vfs_nodes_refcount_sum_get(fs_handle_t fs_handle, service_id_t service_id)
{
	struct refcnt_data rd = {
		.refcnt = 0,
		.fs_handle = fs_handle,
		.service_id = service_id
	};

	fibril_mutex_lock(&nodes_mutex);
	hash_table_apply(&nodes, refcnt_visitor, &rd);
	fibril_mutex_unlock(&nodes_mutex);

	return rd.refcnt;
}


/** Perform a remote node open operation.
 *
 * @return EOK on success or an error code from errno.h.
 *
 */
int vfs_open_node_remote(vfs_node_t *node)
{
	async_exch_t *exch = vfs_exchange_grab(node->fs_handle);
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, VFS_OUT_OPEN_NODE,
	    (sysarg_t) node->service_id, (sysarg_t) node->index, &answer);
	
	vfs_exchange_release(exch);

	sysarg_t rc;
	async_wait_for(req, &rc);
	
	return rc;
}

/**
 * @}
 */
