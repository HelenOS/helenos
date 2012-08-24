/*
 * Copyright (c) 2012 Julia Medvedeva
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
 * @file udf_idx.c
 * @brief Very simple UDF hashtable for nodes
 */

#include "../../vfs/vfs.h"
#include <errno.h>
#include <str.h>
#include <assert.h>
#include <fibril_synch.h>
#include <malloc.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include "udf_idx.h"
#include "udf.h"

#define UDF_IDX_KEYS  2
#define UDF_IDX_KEY   1

#define UDF_IDX_SERVICE_ID_KEY  0
#define UDF_IDX_BUCKETS         1024

static FIBRIL_MUTEX_INITIALIZE(udf_idx_lock);

static hash_table_t udf_idx;

/** Calculate value of hash by key
 *
 * @param Key for calculation of function
 *
 * @return Value of hash function
 *
 */
static hash_index_t udf_idx_hash(unsigned long key[])
{
	/* TODO: This is very simple and probably can be improved */
	return key[UDF_IDX_KEY] % UDF_IDX_BUCKETS;
}

/** Compare two items of hash table
 *
 * @param key  Key of hash table item. Include several items.
 * @param keys Number of parts of key.
 * @param item Item of hash table
 *
 * @return True if input value of key equivalent key of node from hash table
 *
 */
static int udf_idx_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(keys > 0);
	
	udf_node_t *node = hash_table_get_instance(item, udf_node_t, link);
	
	if (node->instance->service_id !=
	    ((service_id_t) key[UDF_IDX_SERVICE_ID_KEY]))
		return false;
	
	assert(keys == 2);
	return (node->index == key[UDF_IDX_KEY]);
}

/** Remove callback
 *
 */
static void udf_idx_remove_cb(link_t *link)
{
	/* We don't use remove callback for this hash table */
}

static hash_table_operations_t udf_idx_ops = {
	.hash = udf_idx_hash,
	.compare = udf_idx_compare,
	.remove_callback = udf_idx_remove_cb,
};

/** Initialization of hash table
 *
 * @return EOK on success or a negative error code.
 *
 */
int udf_idx_init(void)
{
	if (!hash_table_create(&udf_idx, UDF_IDX_BUCKETS, UDF_IDX_KEYS,
	    &udf_idx_ops))
		return ENOMEM;
	
	return EOK;
}

/** Delete hash table
 *
 * @return EOK on success or a negative error code.
 *
 */
int udf_idx_fini(void)
{
	hash_table_destroy(&udf_idx);
	return EOK;
}

/** Get node from hash table
 *
 * @param udfn     Returned value - UDF node
 * @param instance UDF instance
 * @param index    Absolute position of ICB (sector)
 *
 * @return EOK on success or a negative error code.
 *
 */
int udf_idx_get(udf_node_t **udfn, udf_instance_t *instance, fs_index_t index)
{
	fibril_mutex_lock(&udf_idx_lock);
	
	unsigned long key[] = {
		[UDF_IDX_SERVICE_ID_KEY] = instance->service_id,
		[UDF_IDX_KEY] = index
	};
	
	link_t *already_open = hash_table_find(&udf_idx, key);
	if (already_open) {
		udf_node_t *node = hash_table_get_instance(already_open,
		    udf_node_t, link);
		node->ref_cnt++;
		
		*udfn = node;
		
		fibril_mutex_unlock(&udf_idx_lock);
		return EOK;
	}
	
	fibril_mutex_unlock(&udf_idx_lock);
	return ENOENT;
}

/** Create new node in hash table
 *
 * @param udfn     Returned value - new UDF node
 * @param instance UDF instance
 * @param index    Absolute position of ICB (sector)
 *
 * @return EOK on success or a negative error code.
 *
 */
int udf_idx_add(udf_node_t **udfn, udf_instance_t *instance, fs_index_t index)
{
	fibril_mutex_lock(&udf_idx_lock);
	
	udf_node_t *udf_node = malloc(sizeof(udf_node_t));
	if (udf_node == NULL) {
		fibril_mutex_unlock(&udf_idx_lock);
		return ENOMEM;
	}
	
	fs_node_t *fs_node = malloc(sizeof(fs_node_t));
	if (fs_node == NULL) {
		free(udf_node);
		fibril_mutex_unlock(&udf_idx_lock);
		return ENOMEM;
	}
	
	fs_node_initialize(fs_node);
	
	udf_node->index = index;
	udf_node->instance = instance;
	udf_node->ref_cnt = 1;
	udf_node->link_cnt = 0;
	udf_node->fs_node = fs_node;
	udf_node->data = NULL;
	udf_node->allocators = NULL;
	
	fibril_mutex_initialize(&udf_node->lock);
	link_initialize(&udf_node->link);
	fs_node->data = udf_node;
	
	unsigned long key[] = {
		[UDF_IDX_SERVICE_ID_KEY] = instance->service_id,
		[UDF_IDX_KEY] = index
	};
	
	hash_table_insert(&udf_idx, key, &udf_node->link);
	instance->open_nodes_count++;
	
	*udfn = udf_node;
	
	fibril_mutex_unlock(&udf_idx_lock);
	return EOK;
}

/** Delete node from hash table
 *
 * @param node UDF node
 *
 * @return EOK on success or a negative error code.
 *
 */
int udf_idx_del(udf_node_t *node)
{
	assert(node->ref_cnt == 0);
	
	fibril_mutex_lock(&udf_idx_lock);
	
	unsigned long key[] = {
		[UDF_IDX_SERVICE_ID_KEY] = node->instance->service_id,
		[UDF_IDX_KEY] = node->index
	};
	
	hash_table_remove(&udf_idx, key, UDF_IDX_KEYS);
	
	assert(node->instance->open_nodes_count > 0);
	node->instance->open_nodes_count--;
	
	free(node->fs_node);
	free(node);
	
	fibril_mutex_unlock(&udf_idx_lock);
	return EOK;
}

/**
 * @}
 */
