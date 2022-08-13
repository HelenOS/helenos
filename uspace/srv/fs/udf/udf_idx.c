/*
 * SPDX-FileCopyrightText: 2012 Julia Medvedeva
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udf
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
#include <stdlib.h>
#include <adt/hash_table.h>
#include <adt/hash.h>
#include <adt/list.h>
#include <stdbool.h>
#include "udf_idx.h"
#include "udf.h"

static FIBRIL_MUTEX_INITIALIZE(udf_idx_lock);

static hash_table_t udf_idx;

typedef struct {
	service_id_t service_id;
	fs_index_t index;
} udf_ht_key_t;

static size_t udf_idx_hash(const ht_link_t *item)
{
	udf_node_t *node = hash_table_get_inst(item, udf_node_t, link);
	return hash_combine(node->instance->service_id, node->index);
}

static size_t udf_idx_key_hash(const void *k)
{
	const udf_ht_key_t *key = k;
	return hash_combine(key->service_id, key->index);
}

static bool udf_idx_key_equal(const void *k, const ht_link_t *item)
{
	const udf_ht_key_t *key = k;
	udf_node_t *node = hash_table_get_inst(item, udf_node_t, link);

	return (key->service_id == node->instance->service_id) &&
	    (key->index == node->index);
}

static hash_table_ops_t udf_idx_ops = {
	.hash = udf_idx_hash,
	.key_hash = udf_idx_key_hash,
	.key_equal = udf_idx_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

/** Initialization of hash table
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_idx_init(void)
{
	if (!hash_table_create(&udf_idx, 0, 0, &udf_idx_ops))
		return ENOMEM;

	return EOK;
}

/** Delete hash table
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_idx_fini(void)
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
 * @return EOK on success or an error code.
 *
 */
errno_t udf_idx_get(udf_node_t **udfn, udf_instance_t *instance, fs_index_t index)
{
	fibril_mutex_lock(&udf_idx_lock);

	udf_ht_key_t key = {
		.service_id = instance->service_id,
		.index = index
	};

	ht_link_t *already_open = hash_table_find(&udf_idx, &key);
	if (already_open) {
		udf_node_t *node = hash_table_get_inst(already_open,
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
 * @return EOK on success or an error code.
 *
 */
errno_t udf_idx_add(udf_node_t **udfn, udf_instance_t *instance, fs_index_t index)
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
	fs_node->data = udf_node;

	hash_table_insert(&udf_idx, &udf_node->link);
	instance->open_nodes_count++;

	*udfn = udf_node;

	fibril_mutex_unlock(&udf_idx_lock);
	return EOK;
}

/** Delete node from hash table
 *
 * @param node UDF node
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_idx_del(udf_node_t *node)
{
	assert(node->ref_cnt == 0);

	fibril_mutex_lock(&udf_idx_lock);

	hash_table_remove_item(&udf_idx, &node->link);

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
