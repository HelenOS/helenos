/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_HASH_TABLE_H_
#define LIBC_HASH_TABLE_H_

#include <adt/list.h>
#include <unistd.h>
#include <bool.h>


/** Set of operations for hash table. */
typedef struct {
	/** Returns the hash of the key stored in the item.
	 */
	size_t (*hash)(const link_t *item);
	
	/** Returns the hash of the key.
	 */
	size_t (*key_hash)(unsigned long key[]);
	
	/** Hash table item match function.
	 *
	 * @param key Array of keys that will be compared with item. It is
	 *            not necessary to pass all keys.
	 *
	 * @return True if the keys match, false otherwise.
	 *
	 */
	bool (*match)(unsigned long key[], size_t keys, const link_t *item);

	/** 
	 */
	bool (*equal)(const link_t *item1, const link_t *item2);
	
	/** Hash table item removal callback.
	 * 
	 * Must not invoke any mutating functions of the hash table.
	 *
	 * @param item Item that was removed from the hash table.
	 */
	void (*remove_callback)(link_t *item);
} hash_table_ops_t;

/** Hash table structure. */
typedef struct {
	list_t *bucket;
	size_t bucket_cnt;
	size_t max_keys;
	size_t items;
	hash_table_ops_t *op;
} hash_table_t;

#define hash_table_get_instance(item, type, member) \
    list_get_instance((item), type, member)

extern bool hash_table_create(hash_table_t *, size_t, size_t, 
	hash_table_ops_t *);
extern void hash_table_clear(hash_table_t *);
extern void hash_table_insert(hash_table_t *, link_t *);
extern bool hash_table_insert_unique(hash_table_t *, link_t *);
extern link_t *hash_table_find(hash_table_t *, unsigned long []);
extern size_t hash_table_remove(hash_table_t *, unsigned long [], size_t);
extern void hash_table_remove_item(hash_table_t *, link_t *);
extern void hash_table_destroy(hash_table_t *);
extern void hash_table_apply(hash_table_t *, bool (*)(link_t *, void *), 
	void *);

#endif

/** @}
 */
