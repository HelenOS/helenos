/*
 * Copyright (c) 2007 Jakub Jermar
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
#include <string.h>
#include <atomic.h>
#include <futex.h>
#include <libadt/hash_table.h>

/** Futex protecting the VFS node hash table. */
atomic_t nodes_futex = FUTEX_INITIALIZER;

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
	futex_down(&nodes_futex);
	_vfs_node_addref(node);
	futex_up(&nodes_futex);
}

/** Decrement reference count of a VFS node.
 *
 * This function handles the case when the reference count drops to zero.
 *
 * @param node		VFS node that will have its refcnt decremented.
 */
void vfs_node_delref(vfs_node_t *node)
{
	futex_down(&nodes_futex);
	if (node->refcnt-- == 1) {
		unsigned long key[] = {
			[KEY_FS_HANDLE] = node->fs_handle,
			[KEY_DEV_HANDLE] = node->dev_handle,
			[KEY_INDEX] = node->index
		};
		hash_table_remove(&nodes, key, 3);
	}
	futex_up(&nodes_futex);
}

/** Find VFS node.
 *
 * This function will try to lookup the given triplet in the VFS node hash
 * table. In case the triplet is not found there, a new VFS node is created.
 * In any case, the VFS node will have its reference count incremented. Every
 * node returned by this call should be eventually put back by calling
 * vfs_node_put() on it.
 *
 * @param triplet	Triplet encoding the identity of the VFS node.
 *
 * @return		VFS node corresponding to the given triplet.
 */
vfs_node_t *vfs_node_get(vfs_triplet_t *triplet)
{
	unsigned long key[] = {
		[KEY_FS_HANDLE] = triplet->fs_handle,
		[KEY_DEV_HANDLE] = triplet->dev_handle,
		[KEY_INDEX] = triplet->index
	};
	link_t *tmp;
	vfs_node_t *node;

	futex_down(&nodes_futex);
	tmp = hash_table_find(&nodes, key);
	if (!tmp) {
		node = (vfs_node_t *) malloc(sizeof(vfs_node_t));
		if (!node) {
			futex_up(&nodes_futex);
			return NULL;
		}
		memset(node, 0, sizeof(vfs_node_t));
		node->fs_handle = triplet->fs_handle;
		node->dev_handle = triplet->fs_handle;
		node->index = triplet->index;
		link_initialize(&node->nh_link);
		hash_table_insert(&nodes, key, &node->nh_link);
	} else {
		node = hash_table_get_instance(tmp, vfs_node_t, nh_link);	
	}
	_vfs_node_addref(node);
	futex_up(&nodes_futex);

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
	return (node->fs_handle == key[KEY_FS_HANDLE]) &&
	    (node->dev_handle == key[KEY_DEV_HANDLE]) &&
	    (node->index == key[KEY_INDEX]);
}

void nodes_remove_callback(link_t *item)
{
	vfs_node_t *node = hash_table_get_instance(item, vfs_node_t, nh_link);
	free(node);
}

/**
 * @}
 */ 
