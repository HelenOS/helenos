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

#include <libadt/list.h>
#include <unistd.h>

typedef unsigned long hash_count_t;
typedef unsigned long hash_index_t;
typedef struct hash_table hash_table_t;
typedef struct hash_table_operations hash_table_operations_t;

/** Hash table structure. */
struct hash_table {
	link_t *entry;
	hash_count_t entries;
	hash_count_t max_keys;
	hash_table_operations_t *op;
};

/** Set of operations for hash table. */
struct hash_table_operations {
	/** Hash function.
	 *
	 * @param key Array of keys needed to compute hash index. All keys must be passed.
	 *
	 * @return Index into hash table.
	 */
	hash_index_t (* hash)(unsigned long key[]);
	
	/** Hash table item comparison function.
	 *
	 * @param key Array of keys that will be compared with item. It is not necessary to pass all keys.
	 *
	 * @return true if the keys match, false otherwise.
	 */
	int (*compare)(unsigned long key[], hash_count_t keys, link_t *item);

	/** Hash table item removal callback.
	 *
	 * @param item Item that was removed from the hash table.
	 */
	void (*remove_callback)(link_t *item);
};

#define hash_table_get_instance(item, type, member)	list_get_instance((item), type, member)

extern int hash_table_create(hash_table_t *h, hash_count_t m, hash_count_t max_keys, hash_table_operations_t *op);
extern void hash_table_insert(hash_table_t *h, unsigned long key[], link_t *item);
extern link_t *hash_table_find(hash_table_t *h, unsigned long key[]);
extern void hash_table_remove(hash_table_t *h, unsigned long key[], hash_count_t keys);

#endif

/** @}
 */
