/*
 * Copyright (C) 2006 Jakub Jermar
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

/*
 * This is an implementation of generic chained hash table.
 */

#include <hash_table.h>
#include <list.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

/** Create chained hash table.
 *
 * @param h Hash table structure. Will be initialized by this call.
 * @param m Number of slots in the hash table.
 * @param max_keys Maximal number of keys needed to identify an item.
 * @param op Hash table operations structure.
 * @return true on success
 */
int hash_table_create(hash_table_t *h, hash_count_t m, hash_count_t max_keys, hash_table_operations_t *op)
{
	hash_count_t i;

	assert(h);
	assert(op && op->hash && op->compare);
	assert(max_keys > 0);
	
	h->entry = malloc(m * sizeof(link_t));
	if (!h->entry) {
		printf("cannot allocate memory for hash table\n");
		return false;
	}
	memset((void *) h->entry, 0,  m * sizeof(link_t));
	
	for (i = 0; i < m; i++)
		list_initialize(&h->entry[i]);
	
	h->entries = m;
	h->max_keys = max_keys;
	h->op = op;
	return true;
}

/** Insert item into hash table.
 *
 * @param h Hash table.
 * @param hey Array of all keys necessary to compute hash index.
 * @param item Item to be inserted into the hash table.
 */
void hash_table_insert(hash_table_t *h, unsigned long key[], link_t *item)
{
	hash_index_t chain;

	assert(item);
	assert(h && h->op && h->op->hash && h->op->compare);

	chain = h->op->hash(key);
	assert(chain < h->entries);
	
	list_append(item, &h->entry[chain]);
}

/** Search hash table for an item matching keys.
 *
 * @param h Hash table.
 * @param key Array of all keys needed to compute hash index.
 *
 * @return Matching item on success, NULL if there is no such item.
 */
link_t *hash_table_find(hash_table_t *h, unsigned long key[])
{
	link_t *cur;
	hash_index_t chain;

	assert(h && h->op && h->op->hash && h->op->compare);

	chain = h->op->hash(key);
	assert(chain < h->entries);
	
	/*
	 * The hash table is not redundant.
	 * Check if the keys are not in place already.
	 */
	for (cur = h->entry[chain].next; cur != &h->entry[chain]; cur = cur->next) {
		if (h->op->compare(key, h->max_keys, cur)) {
			/*
			 * The entry is there.
			 */
			return cur;
		}
	}
	
	return NULL;
}

/** Remove all matching items from hash table.
 *
 * For each removed item, h->remove_callback() is called.
 *
 * @param h Hash table.
 * @param key Array of keys that will be compared against items of the hash table.
 * @param keys Number of keys in the 'key' array.
 */
void hash_table_remove(hash_table_t *h, unsigned long key[], hash_count_t keys)
{
	hash_index_t chain;
	link_t *cur;

	assert(h && h->op && h->op->hash && h->op->compare && h->op->remove_callback);
	assert(keys <= h->max_keys);
	
	if (keys == h->max_keys) {

		/*
		 * All keys are known, hash_table_find() can be used to find the entry.
		 */
	
		cur = hash_table_find(h, key);
		if (cur) {
			list_remove(cur);
			h->op->remove_callback(cur);
		}
		return;
	}
	
	/*
	 * Fewer keys were passed.
	 * Any partially matching entries are to be removed.
	 */
	for (chain = 0; chain < h->entries; chain++) {
		for (cur = h->entry[chain].next; cur != &h->entry[chain]; cur = cur->next) {
			if (h->op->compare(key, keys, cur)) {
				link_t *hlp;
				
				hlp = cur;
				cur = cur->prev;
				
				list_remove(hlp);
				h->op->remove_callback(hlp);
				
				continue;
			}
		}
	}
}
