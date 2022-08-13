/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERNEL_HASH_TABLE_H_
#define KERNEL_HASH_TABLE_H_

#include <adt/list.h>
#include <stdbool.h>
#include <macros.h>
#include <member.h>

/** Opaque hash table link type. */
typedef struct ht_link {
	link_t link;
} ht_link_t;

/** Set of operations for hash table. */
typedef struct {
	/** Returns the hash of the key stored in the item (ie its lookup key). */
	size_t (*hash)(const ht_link_t *item);

	/** Returns the hash of the key. */
	size_t (*key_hash)(const void *key);

	/** True if the items are equal (have the same lookup keys). */
	bool (*equal)(const ht_link_t *item1, const ht_link_t *item2);

	/** Returns true if the key is equal to the item's lookup key. */
	bool (*key_equal)(const void *key, const ht_link_t *item);

	/** Hash table item removal callback.
	 *
	 * Must not invoke any mutating functions of the hash table.
	 *
	 * @param item Item that was removed from the hash table.
	 */
	void (*remove_callback)(ht_link_t *item);
} hash_table_ops_t;

/** Hash table structure. */
typedef struct {
	hash_table_ops_t *op;
	list_t *bucket;
	size_t bucket_cnt;
	size_t full_item_cnt;
	size_t item_cnt;
	size_t max_load;
	bool apply_ongoing;
} hash_table_t;

#define hash_table_get_inst(item, type, member) \
	member_to_inst((item), type, member)

extern bool hash_table_create(hash_table_t *, size_t, size_t,
    hash_table_ops_t *);
extern void hash_table_destroy(hash_table_t *);

extern bool hash_table_empty(hash_table_t *);
extern size_t hash_table_size(hash_table_t *);

extern void hash_table_clear(hash_table_t *);
extern void hash_table_insert(hash_table_t *, ht_link_t *);
extern bool hash_table_insert_unique(hash_table_t *, ht_link_t *);
extern ht_link_t *hash_table_find(const hash_table_t *, const void *);
extern ht_link_t *hash_table_find_next(const hash_table_t *, ht_link_t *,
    ht_link_t *);
extern size_t hash_table_remove(hash_table_t *, const void *);
extern void hash_table_remove_item(hash_table_t *, ht_link_t *);
extern void hash_table_apply(hash_table_t *, bool (*)(ht_link_t *, void *),
    void *);

#endif

/** @}
 */
