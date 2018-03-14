/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2012 Adam Hraska
 *
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
#include <mm/slab.h>
#include <assert.h>
#include <str.h>

/* Optimal initial bucket count. See comment above. */
#define HT_MIN_BUCKETS  89
/* The table is resized when the average load per bucket exceeds this number. */
#define HT_MAX_LOAD     2


static size_t round_up_size(size_t);
static bool alloc_table(size_t, list_t **);
static void clear_items(hash_table_t *);
static void resize(hash_table_t *, size_t);
static void grow_if_needed(hash_table_t *);
static void shrink_if_needed(hash_table_t *);

/* Dummy do nothing callback to invoke in place of remove_callback == NULL. */
static void nop_remove_callback(ht_link_t *item)
{
	/* no-op */
}


/** Create chained hash table.
 *
 * @param h        Hash table structure. Will be initialized by this call.
 * @param init_size Initial desired number of hash table buckets. Pass zero
 *                 if you want the default initial size.
 * @param max_load The table is resized when the average load per bucket
 *                 exceeds this number. Pass zero if you want the default.
 * @param op       Hash table operations structure. remove_callback()
 *                 is optional and can be NULL if no action is to be taken
 *                 upon removal. equal() is optional if and only if
 *                 hash_table_insert_unique() will never be invoked.
 *                 All other operations are mandatory.
 *
 * @return True on success
 *
 */
bool hash_table_create(hash_table_t *h, size_t init_size, size_t max_load,
    hash_table_ops_t *op)
{
	assert(h);
	assert(op && op->hash && op->key_hash && op->key_equal);

	/* Check for compulsory ops. */
	if (!op || !op->hash || !op->key_hash || !op->key_equal)
		return false;

	h->bucket_cnt = round_up_size(init_size);

	if (!alloc_table(h->bucket_cnt, &h->bucket))
		return false;

	h->max_load = (max_load == 0) ? HT_MAX_LOAD : max_load;
	h->item_cnt = 0;
	h->op = op;
	h->full_item_cnt = h->max_load * h->bucket_cnt;
	h->apply_ongoing = false;

	if (h->op->remove_callback == NULL) {
		h->op->remove_callback = nop_remove_callback;
	}

	return true;
}

/** Destroy a hash table instance.
 *
 * @param h Hash table to be destroyed.
 *
 */
void hash_table_destroy(hash_table_t *h)
{
	assert(h && h->bucket);
	assert(!h->apply_ongoing);

	clear_items(h);

	free(h->bucket);

	h->bucket = NULL;
	h->bucket_cnt = 0;
}

/** Returns true if there are no items in the table. */
bool hash_table_empty(hash_table_t *h)
{
	assert(h && h->bucket);
	return h->item_cnt == 0;
}

/** Returns the number of items in the table. */
size_t hash_table_size(hash_table_t *h)
{
	assert(h && h->bucket);
	return h->item_cnt;
}

/** Remove all elements from the hash table
 *
 * @param h Hash table to be cleared
 */
void hash_table_clear(hash_table_t *h)
{
	assert(h && h->bucket);
	assert(!h->apply_ongoing);

	clear_items(h);

	/* Shrink the table to its minimum size if possible. */
	if (HT_MIN_BUCKETS < h->bucket_cnt) {
		resize(h, HT_MIN_BUCKETS);
	}
}

/** Unlinks and removes all items but does not resize. */
static void clear_items(hash_table_t *h)
{
	if (h->item_cnt == 0)
		return;

	for (size_t idx = 0; idx < h->bucket_cnt; ++idx) {
		list_foreach_safe(h->bucket[idx], cur, next) {
			assert(cur);
			ht_link_t *cur_link = member_to_inst(cur, ht_link_t, link);

			list_remove(cur);
			h->op->remove_callback(cur_link);
		}
	}

	h->item_cnt = 0;
}

/** Insert item into a hash table.
 *
 * @param h    Hash table.
 * @param item Item to be inserted into the hash table.
 */
void hash_table_insert(hash_table_t *h, ht_link_t *item)
{
	assert(item);
	assert(h && h->bucket);
	assert(!h->apply_ongoing);

	size_t idx = h->op->hash(item) % h->bucket_cnt;

	list_append(&item->link, &h->bucket[idx]);
	++h->item_cnt;
	grow_if_needed(h);
}


/** Insert item into a hash table if not already present.
 *
 * @param h    Hash table.
 * @param item Item to be inserted into the hash table.
 *
 * @return False if such an item had already been inserted.
 * @return True if the inserted item was the only item with such a lookup key.
 */
bool hash_table_insert_unique(hash_table_t *h, ht_link_t *item)
{
	assert(item);
	assert(h && h->bucket && h->bucket_cnt);
	assert(h->op && h->op->hash && h->op->equal);
	assert(!h->apply_ongoing);

	size_t idx = h->op->hash(item) % h->bucket_cnt;

	/* Check for duplicates. */
	list_foreach(h->bucket[idx], link, ht_link_t, cur_link) {
		/*
		 * We could filter out items using their hashes first, but
		 * calling equal() might very well be just as fast.
		 */
		if (h->op->equal(cur_link, item))
			return false;
	}

	list_append(&item->link, &h->bucket[idx]);
	++h->item_cnt;
	grow_if_needed(h);

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
ht_link_t *hash_table_find(const hash_table_t *h, void *key)
{
	assert(h && h->bucket);

	size_t idx = h->op->key_hash(key) % h->bucket_cnt;

	list_foreach(h->bucket[idx], link, ht_link_t, cur_link) {
		/*
		 * Is this is the item we are looking for? We could have first
		 * checked if the hashes match but op->key_equal() may very well be
		 * just as fast as op->hash().
		 */
		if (h->op->key_equal(key, cur_link)) {
			return cur_link;
		}
	}

	return NULL;
}

/** Find the next item equal to item. */
ht_link_t *
hash_table_find_next(const hash_table_t *h, ht_link_t *first, ht_link_t *item)
{
	assert(item);
	assert(h && h->bucket);

	size_t idx = h->op->hash(item) % h->bucket_cnt;

	/* Traverse the circular list until we reach the starting item again. */
	for (link_t *cur = item->link.next; cur != &first->link;
	    cur = cur->next) {
		assert(cur);

		if (cur == &h->bucket[idx].head)
			continue;

		ht_link_t *cur_link = member_to_inst(cur, ht_link_t, link);
		/*
		 * Is this is the item we are looking for? We could have first
		 * checked if the hashes match but op->equal() may very well be
		 * just as fast as op->hash().
		 */
		if (h->op->equal(cur_link, item)) {
			return cur_link;
		}
	}

	return NULL;
}

/** Remove all matching items from hash table.
 *
 * For each removed item, h->remove_callback() is called.
 *
 * @param h    Hash table.
 * @param key  Array of keys that will be compared against items of
 *             the hash table.
 *
 * @return Returns the number of removed items.
 */
size_t hash_table_remove(hash_table_t *h, void *key)
{
	assert(h && h->bucket);
	assert(!h->apply_ongoing);

	size_t idx = h->op->key_hash(key) % h->bucket_cnt;

	size_t removed = 0;

	list_foreach_safe(h->bucket[idx], cur, next) {
		ht_link_t *cur_link = member_to_inst(cur, ht_link_t, link);

		if (h->op->key_equal(key, cur_link)) {
			++removed;
			list_remove(cur);
			h->op->remove_callback(cur_link);
		}
	}

	h->item_cnt -= removed;
	shrink_if_needed(h);

	return removed;
}

/** Removes an item already present in the table. The item must be in the table.*/
void hash_table_remove_item(hash_table_t *h, ht_link_t *item)
{
	assert(item);
	assert(h && h->bucket);
	assert(link_in_use(&item->link));

	list_remove(&item->link);
	--h->item_cnt;
	h->op->remove_callback(item);
	shrink_if_needed(h);
}

/** Apply function to all items in hash table.
 *
 * @param h   Hash table.
 * @param f   Function to be applied. Return false if no more items
 *            should be visited. The functor may only delete the supplied
 *            item. It must not delete the successor of the item passed
 *            in the first argument.
 * @param arg Argument to be passed to the function.
 */
void hash_table_apply(hash_table_t *h, bool (*f)(ht_link_t *, void *), void *arg)
{
	assert(f);
	assert(h && h->bucket);

	if (h->item_cnt == 0)
		return;

	h->apply_ongoing = true;

	for (size_t idx = 0; idx < h->bucket_cnt; ++idx) {
		list_foreach_safe(h->bucket[idx], cur, next) {
			ht_link_t *cur_link = member_to_inst(cur, ht_link_t, link);
			/*
			 * The next pointer had already been saved. f() may safely
			 * delete cur (but not next!).
			 */
			if (!f(cur_link, arg))
				goto out;
		}
	}
out:
	h->apply_ongoing = false;

	shrink_if_needed(h);
	grow_if_needed(h);
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

	list_t *buckets = malloc(bucket_cnt * sizeof(list_t), FRAME_ATOMIC);
	if (!buckets)
		return false;

	for (size_t i = 0; i < bucket_cnt; i++)
		list_initialize(&buckets[i]);

	*pbuckets = buckets;
	return true;
}


/** Shrinks the table if the table is only sparely populated. */
static inline void shrink_if_needed(hash_table_t *h)
{
	if (h->item_cnt <= h->full_item_cnt / 4 && HT_MIN_BUCKETS < h->bucket_cnt) {
		/*
		 * Keep the bucket_cnt odd (possibly also prime).
		 * Shrink from 2n + 1 to n. Integer division discards the +1.
		 */
		size_t new_bucket_cnt = h->bucket_cnt / 2;
		resize(h, new_bucket_cnt);
	}
}

/** Grows the table if table load exceeds the maximum allowed. */
static inline void grow_if_needed(hash_table_t *h)
{
	/* Grow the table if the average bucket load exceeds the maximum. */
	if (h->full_item_cnt < h->item_cnt) {
		/* Keep the bucket_cnt odd (possibly also prime). */
		size_t new_bucket_cnt = 2 * h->bucket_cnt + 1;
		resize(h, new_bucket_cnt);
	}
}

/** Allocates and rehashes items to a new table. Frees the old table. */
static void resize(hash_table_t *h, size_t new_bucket_cnt)
{
	assert(h && h->bucket);
	assert(HT_MIN_BUCKETS <= new_bucket_cnt);

	/* We are traversing the table and resizing would mess up the buckets. */
	if (h->apply_ongoing)
		return;

	list_t *new_buckets;

	/* Leave the table as is if we cannot resize. */
	if (!alloc_table(new_bucket_cnt, &new_buckets))
		return;

	if (0 < h->item_cnt) {
		/* Rehash all the items to the new table. */
		for (size_t old_idx = 0; old_idx < h->bucket_cnt; ++old_idx) {
			list_foreach_safe(h->bucket[old_idx], cur, next) {
				ht_link_t *cur_link = member_to_inst(cur, ht_link_t, link);

				size_t new_idx = h->op->hash(cur_link) % new_bucket_cnt;
				list_remove(cur);
				list_append(cur, &new_buckets[new_idx]);
			}
		}
	}

	free(h->bucket);
	h->bucket = new_buckets;
	h->bucket_cnt = new_bucket_cnt;
	h->full_item_cnt = h->max_load * h->bucket_cnt;
}


/** @}
 */
