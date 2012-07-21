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

/** @addtogroup libc
 * @{
 */
/** @file
 */

/*
 * This is an implementation of a generic resizable chained hash table.
 * 
 * The table grows to 2*n+1 buckets each time, starting at n == 89, 
 * per Thomas Wang's recommendation:
 * http://www.concentric.net/~Ttwang/tech/hashsize.htm
 * 
 * This policy produces prime table sizes for the first five resizes
 * and generally produces table sizes which are either prime or 
 * have fairly large (prime/odd) divisors. Having a prime table size
 * mitigates the use of suboptimal hash functions and distributes
 * items over the whole table.
 */

#include <adt/hash_table.h>
#include <adt/list.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <str.h>

/* Optimal initial bucket count. See comment above. */
#define HT_MIN_BUCKETS  89
/* The table is resized when the average load per bucket exceeds this number. */
#define HT_MAX_LOAD     2


static size_t round_up_size(size_t size);
static bool alloc_table(size_t bucket_cnt, list_t **pbuckets);
static void item_inserted(hash_table_t *h);
static void item_removed(hash_table_t *h);
static inline void remove_item(hash_table_t *h, link_t *item);
static size_t remove_duplicates(hash_table_t *h, unsigned long key[]);
static size_t remove_matching(hash_table_t *h, unsigned long key[], size_t key_cnt);

/* Dummy do nothing callback to invoke in place of remove_callback == NULL. */
static void nop_remove_callback(link_t *item)
{
	/* no-op */
}


/** Create chained hash table.
 *
 * @param h        Hash table structure. Will be initialized by this call.
 * @param init_size Initial desired number of hash table buckets. Pass zero
 *                 if you want the default initial size. 
 * @param max_keys Maximal number of keys needed to identify an item.
 * @param op       Hash table operations structure. remove_callback()
 *                 is optional and can be NULL if no action is to be taken
 *                 upon removal. equal() is optional if and only if
 *                 hash_table_insert_unique() will never be invoked.
 *                 All other operations are mandatory. 
 *
 * @return True on success
 *
 */
bool hash_table_create(hash_table_t *h, size_t init_size, size_t max_keys,
    hash_table_ops_t *op)
{
	assert(h);
	assert(op && op->hash && op->key_hash && op->match);
	assert(max_keys > 0);
	
	h->bucket_cnt = round_up_size(init_size);
	
	if (!alloc_table(h->bucket_cnt, &h->bucket))
		return false;
	
	h->max_keys = max_keys;
	h->items = 0;
	h->op = op;
	
	if (h->op->remove_callback == 0) 
		h->op->remove_callback = nop_remove_callback;
	
	return true;
}

/** Remove all elements from the hash table
 *
 * @param h Hash table to be cleared
 */
void hash_table_clear(hash_table_t *h)
{
	for (size_t idx = 0; idx < h->bucket_cnt; ++idx) {
		list_foreach_safe(h->bucket[idx], cur, next) {
			list_remove(cur);
			h->op->remove_callback(cur);
		}
	}
	
	h->items = 0;

	/* Shrink the table to its minimum size if possible. */
	if (HT_MIN_BUCKETS < h->bucket_cnt) {
		list_t *new_buckets;
		if (alloc_table(HT_MIN_BUCKETS, &new_buckets)) {
			free(h->bucket);
			h->bucket = new_buckets;
			h->bucket_cnt = HT_MIN_BUCKETS;
		}
	}
}

/** Destroy a hash table instance.
 *
 * @param h Hash table to be destroyed.
 *
 */
void hash_table_destroy(hash_table_t *h)
{
	assert(h);
	assert(h->bucket);
	
	free(h->bucket);

	h->bucket = 0;
	h->bucket_cnt = 0;
}

/** Insert item into a hash table.
 *
 * @param h    Hash table.
 * @param key  Array of all keys necessary to compute hash index.
 * @param item Item to be inserted into the hash table.
 */
void hash_table_insert(hash_table_t *h, link_t *item)
{
	assert(item);
	assert(h && h->bucket);
	assert(h->op && h->op->hash);
	
	size_t idx = h->op->hash(item) % h->bucket_cnt;
	
	assert(idx < h->bucket_cnt);
	
	list_append(item, &h->bucket[idx]);
	item_inserted(h);
}


/** Insert item into a hash table if not already present.
 *
 * @param h    Hash table.
 * @param key  Array of all keys necessary to compute hash index.
 * @param item Item to be inserted into the hash table.
 * 
 * @return False if such an item had already been inserted. 
 * @return True if the inserted item was the only item with such a lookup key.
 */
bool hash_table_insert_unique(hash_table_t *h, link_t *item)
{
	assert(item);
	assert(h && h->bucket && h->bucket_cnt);
	assert(h->op && h->op->hash && h->op->equal);
	
	size_t item_hash = h->op->hash(item);
	size_t idx = item_hash % h->bucket_cnt;
	
	assert(idx < h->bucket_cnt);
	
	/* Check for duplicates. */
	list_foreach(h->bucket[idx], cur) {
		/* 
		 * We could filter out items using their hashes first, but 
		 * calling equal() might very well be just as fast.
		 */
		if (h->op->equal(cur, item))
			return false;
	}
	
	list_append(item, &h->bucket[idx]);
	item_inserted(h);
	
	return true;
}

/** Search hash table for an item matching keys.
 *
 * @param h   Hash table.
 * @param key Array of all keys needed to compute hash index.
 *
 * @return Matching item on success, NULL if there is no such item.
 *
 */
link_t *hash_table_find(hash_table_t *h, unsigned long key[])
{
	assert(h && h->bucket);
	assert(h->op && h->op->key_hash && h->op->match);
	
	size_t key_hash = h->op->key_hash(key);
	size_t idx = key_hash % h->bucket_cnt;

	assert(idx < h->bucket_cnt);
	
	list_foreach(h->bucket[idx], cur) {
		/* 
		 * Is this is the item we are looking for? We could have first 
		 * checked if the hashes match but op->match() may very well be 
		 * just as fast as op->hash().
		 */
		if (h->op->match(key, h->max_keys, cur)) {
			return cur;
		}
	}
	
	return NULL;
}


/** Apply function to all items in hash table.
 *
 * @param h   Hash table.
 * @param f   Function to be applied. Return false if no more items 
 *            should be visited. The functor must not delete the successor
 *            of the item passed in the first argument.
 * @param arg Argument to be passed to the function.
 *
 */
void hash_table_apply(hash_table_t *h, bool (*f)(link_t *, void *), void *arg)
{	
	for (size_t idx = 0; idx < h->bucket_cnt; ++idx) {
		list_foreach_safe(h->bucket[idx], cur, next) {
			/* 
			 * The next pointer had already been saved. f() may safely 
			 * delete cur (but not next!).
			 */
			if (!f(cur, arg))
				return;
		}
	}
}

/** Remove all matching items from hash table.
 *
 * For each removed item, h->remove_callback() is called.
 *
 * @param h    Hash table.
 * @param key  Array of keys that will be compared against items of
 *             the hash table.
 * @param keys Number of keys in the 'key' array.
 * 
 * @return Returns the number of removed items.
 */
size_t hash_table_remove(hash_table_t *h, unsigned long key[], size_t key_cnt)
{
	assert(h && h->bucket);
	assert(h && h->op && h->op->hash &&
	    h->op->remove_callback);
	assert(key_cnt <= h->max_keys);
	
	/* All keys are known, remove from a specific bucket. */
	if (key_cnt == h->max_keys) {
		return remove_duplicates(h, key);
	} else {
		/*
		* Fewer keys were passed.
		* Any partially matching entries are to be removed.
		*/
		return remove_matching(h, key, key_cnt);
	}
}

/** Removes an item already present in the table. The item must be in the table.*/
void hash_table_remove_item(hash_table_t *h, link_t *item)
{
	assert(item);
	assert(h && h->bucket);
	
	remove_item(h, item);
}

/** Unlink the item from a bucket, update statistics and resize if needed. */
static inline void remove_item(hash_table_t *h, link_t *item)
{
	assert(item);
	
	list_remove(item);
	item_removed(h);
	h->op->remove_callback(item);
}

/** Removes all items matching key in the bucket key hashes to. */
static size_t remove_duplicates(hash_table_t *h, unsigned long key[])
{
	assert(h && h->bucket);
	assert(h->op && h->op->key_hash && h->op->match);
	
	size_t key_hash = h->op->key_hash(key);
	size_t idx = key_hash % h->bucket_cnt;

	assert(idx < h->bucket_cnt);
	
	size_t removed = 0;
	
	list_foreach_safe(h->bucket[idx], cur, next) {
		if (h->op->match(key, h->max_keys, cur)) {
			++removed;
			remove_item(h, cur);
		}
	}
	
	return removed;
}

/** Removes all items in any bucket in the table that match the partial key. */
static size_t remove_matching(hash_table_t *h, unsigned long key[], 
	size_t key_cnt)
{
	assert(h && h->bucket);
	assert(key_cnt < h->max_keys);
	
	size_t removed = 0;
	/*
	 * Fewer keys were passed.
	 * Any partially matching entries are to be removed.
	 */
	for (size_t idx = 0; idx < h->bucket_cnt; ++idx) {
		list_foreach_safe(h->bucket[idx], cur, next) {
			if (h->op->match(key, key_cnt, cur)) {
				++removed;
				remove_item(h, cur);
			}
		}
	}
	
	return removed;
	
}

/** Rounds up size to the nearest suitable table size. */
static size_t round_up_size(size_t size)
{
	size_t rounded_size = HT_MIN_BUCKETS;
	
	while (rounded_size < size) {
		rounded_size = 2 * rounded_size + 1;
	}
	
	return rounded_size;
}

/** Allocates and initializes the desired number of buckets. True if successful.*/
static bool alloc_table(size_t bucket_cnt, list_t **pbuckets)
{
	assert(pbuckets && HT_MIN_BUCKETS <= bucket_cnt);
		
	list_t *buckets = malloc(bucket_cnt * sizeof(list_t));
	if (!buckets)
		return false;
	
	for (size_t i = 0; i < bucket_cnt; i++)
		list_initialize(&buckets[i]);

	*pbuckets = buckets;
	return true;
}

/** Allocates and rehashes items to a new table. Frees the old table. */
static void resize(hash_table_t *h, size_t new_bucket_cnt) 
{
	assert(h && h->bucket);
	
	list_t *new_buckets;

	/* Leave the table as is if we cannot resize. */
	if (!alloc_table(new_bucket_cnt, &new_buckets))
		return;
	
	/* Rehash all the items to the new table. */
	for (size_t old_idx = 0; old_idx < h->bucket_cnt; ++old_idx) {
		list_foreach_safe(h->bucket[old_idx], cur, next) {
			size_t new_idx = h->op->hash(cur) % new_bucket_cnt;
			list_remove(cur);
			list_append(cur, &new_buckets[new_idx]);
		}
	}
	
	free(h->bucket);
	h->bucket = new_buckets;
	h->bucket_cnt = new_bucket_cnt;
}

/** Shrinks the table if needed. */
static void item_removed(hash_table_t *h)
{
	--h->items;
	
	if (HT_MIN_BUCKETS < h->items && h->items <= HT_MAX_LOAD * h->bucket_cnt / 4) {
		/* 
		 * Keep the bucket_cnt odd (possibly also prime). 
		 * Shrink from 2n + 1 to n. Integer division discards the +1.
		 */
		size_t new_bucket_cnt = h->bucket_cnt / 2;
		resize(h, new_bucket_cnt);
	}
}

/** Grows the table if needed. */
static void item_inserted(hash_table_t *h)
{
	++h->items;
	
	/* Grow the table if the average bucket load exceeds the maximum. */
	if (HT_MAX_LOAD * h->bucket_cnt < h->items) {
		/* Keep the bucket_cnt odd (possibly also prime). */
		size_t new_bucket_cnt = 2 * h->bucket_cnt + 1;
		resize(h, new_bucket_cnt);
	}
}


/** @}
 */
