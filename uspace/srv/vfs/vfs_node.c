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
#include <adt/hash.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <macros.h>

/** Mutex protecting the VFS node hash table. */
FIBRIL_MUTEX_INITIALIZE(nodes_mutex);

#define NODES_BUCKETS_LOG	8
#define NODES_BUCKETS		(1 << NODES_BUCKETS_LOG)

/** VFS node hash table containing all active, in-memory VFS nodes. */
hash_table_t nodes;

#define KEY_FS_HANDLE	0
#define KEY_DEV_HANDLE	1
#define KEY_INDEX	2

static size_t nodes_key_hash(void *);
static size_t nodes_hash(const ht_link_t *);
static bool nodes_key_equal(void *, const ht_link_t *);
static vfs_triplet_t node_triplet(vfs_node_t *node);

/** VFS node hash table operations. */
hash_table_ops_t nodes_ops = {
	.hash = nodes_hash,
	.key_hash = nodes_key_hash,
	.key_equal = nodes_key_equal,
	.equal = NULL,
	.remove_callback = NULL,
};

/** Initialize the VFS node hash table.
 *
 * @return		Return true on success, false on failure.
 */
bool vfs_nodes_init(void)
{
	return hash_table_create(&nodes, 0, 0, &nodes_ops);
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
	bool free_node = false;

	fibril_mutex_lock(&nodes_mutex);

	node->refcnt--;
	if (node->refcnt == 0) {
		/*
		 * We are dropping the last reference to this node.
		 * Remove it from the VFS node hash table.
		 */

		hash_table_remove_item(&nodes, &node->nh_link);
		free_node = true;
	}

	fibril_mutex_unlock(&nodes_mutex);

	if (free_node) {
		/*
		 * VFS_OUT_DESTROY will free up the file's resources if there
		 * are no more hard links.
		 */

		async_exch_t *exch = vfs_exchange_grab(node->fs_handle);
		async_msg_2(exch, VFS_OUT_DESTROY, (sysarg_t) node->service_id,
		    (sysarg_t)node->index);
		vfs_exchange_release(exch);

		free(node);
	}
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
	hash_table_remove_item(&nodes, &node->nh_link);
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
	vfs_node_t *node;

	fibril_mutex_lock(&nodes_mutex);
	ht_link_t *tmp = hash_table_find(&nodes, &result->triplet);
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
		node->type = result->type;
		fibril_rwlock_initialize(&node->contents_rwlock);
		hash_table_insert(&nodes, &node->nh_link);
	} else {
		node = hash_table_get_inst(tmp, vfs_node_t, nh_link);
	}

	_vfs_node_addref(node);
	fibril_mutex_unlock(&nodes_mutex);

	return node;
}

vfs_node_t *vfs_node_peek(vfs_lookup_res_t *result)
{
	vfs_node_t *node = NULL;

	fibril_mutex_lock(&nodes_mutex);
	ht_link_t *tmp = hash_table_find(&nodes, &result->triplet);
	if (tmp) {
		node = hash_table_get_inst(tmp, vfs_node_t, nh_link);
		_vfs_node_addref(node);
	}
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

struct refcnt_data {
	/** Sum of all reference counts for this file system instance. */
	unsigned refcnt;
	fs_handle_t fs_handle;
	service_id_t service_id;
};

static bool refcnt_visitor(ht_link_t *item, void *arg)
{
	vfs_node_t *node = hash_table_get_inst(item, vfs_node_t, nh_link);
	struct refcnt_data *rd = (void *) arg;

	if ((node->fs_handle == rd->fs_handle) &&
	    (node->service_id == rd->service_id))
		rd->refcnt += node->refcnt;

	return true;
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
errno_t vfs_open_node_remote(vfs_node_t *node)
{
	async_exch_t *exch = vfs_exchange_grab(node->fs_handle);

	ipc_call_t answer;
	aid_t req = async_send_2(exch, VFS_OUT_OPEN_NODE,
	    (sysarg_t) node->service_id, (sysarg_t) node->index, &answer);

	vfs_exchange_release(exch);

	errno_t rc;
	async_wait_for(req, &rc);

	return rc;
}


static size_t nodes_key_hash(void *key)
{
	vfs_triplet_t *tri = key;
	size_t hash = hash_combine(tri->fs_handle, tri->index);
	return hash_combine(hash, tri->service_id);
}

static size_t nodes_hash(const ht_link_t *item)
{
	vfs_node_t *node = hash_table_get_inst(item, vfs_node_t, nh_link);
	vfs_triplet_t tri = node_triplet(node);
	return nodes_key_hash(&tri);
}

static bool nodes_key_equal(void *key, const ht_link_t *item)
{
	vfs_triplet_t *tri = key;
	vfs_node_t *node = hash_table_get_inst(item, vfs_node_t, nh_link);
	return node->fs_handle == tri->fs_handle &&
	    node->service_id == tri->service_id && node->index == tri->index;
}

static inline vfs_triplet_t node_triplet(vfs_node_t *node)
{
	vfs_triplet_t tri = {
		.fs_handle = node->fs_handle,
		.service_id = node->service_id,
		.index = node->index
	};

	return tri;
}

bool vfs_node_has_children(vfs_node_t *node)
{
	async_exch_t *exch = vfs_exchange_grab(node->fs_handle);
	errno_t rc = async_req_2_0(exch, VFS_OUT_IS_EMPTY, node->service_id,
	    node->index);
	vfs_exchange_release(exch);
	return rc == ENOTEMPTY;
}

/**
 * @}
 */
