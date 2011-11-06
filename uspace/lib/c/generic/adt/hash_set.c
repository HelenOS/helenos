/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Radim Vansa
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

#include <adt/hash_set.h>
#include <adt/list.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <str.h>

/** Create chained hash set
 *
 * @param     h         Hash set structure to be initialized.
 * @param[in] hash      Hash function
 * @param[in] equals    Equals function
 * @param[in] init_size Initial hash set size
 *
 * @return True on success
 *
 */
int hash_set_init(hash_set_t *h, hash_set_hash hash, hash_set_equals equals,
    size_t init_size)
{
	assert(h);
	assert(hash);
	assert(equals);
	
	if (init_size < HASH_SET_MIN_SIZE)
		init_size = HASH_SET_MIN_SIZE;
	
	h->table = malloc(init_size * sizeof(link_t));
	if (!h->table)
		return false;
	
	for (size_t i = 0; i < init_size; i++)
		list_initialize(&h->table[i]);
	
	h->size = init_size;
	h->count = 0;
	h->hash = hash;
	h->equals = equals;
	
	return true;
}

/** Destroy a hash table instance.
 *
 * @param h Hash table to be destroyed.
 *
 */
void hash_set_destroy(hash_set_t *h)
{
	assert(h);
	free(h->table);
}

/** Rehash the internal table to new table
 *
 * @param h         Original hash set
 * @param new_table Memory for the new table
 * @param new_size  Size of the new table
 */
static void hash_set_rehash(hash_set_t *h, list_t *new_table,
    size_t new_size)
{
	assert(new_size >= HASH_SET_MIN_SIZE);
	
	for (size_t bucket = 0; bucket < new_size; bucket++)
		list_initialize(&new_table[bucket]);
	
	for (size_t bucket = 0; bucket < h->size; bucket++) {
		link_t *cur;
		link_t *next;
		
		for (cur = h->table[bucket].head.next;
		    cur != &h->table[bucket].head;
		    cur = next) {
			next = cur->next;
			list_append(cur, &new_table[h->hash(cur) % new_size]);
		}
	}
	
	list_t *old_table = h->table;
	h->table = new_table;
	free(old_table);
	h->size = new_size;
}

/** Insert item into the set.
 *
 * If the set already contains equivalent object,
 * the function fails.
 *
 * @param h    Hash table.
 * @param key  Array of all keys necessary to compute hash index.
 * @param item Item to be inserted into the hash table.
 *
 * @return True if the object was inserted
 * @return Ffalse if the set already contained equivalent object.
 *
 */
int hash_set_insert(hash_set_t *h, link_t *item)
{
	assert(item);
	assert(h);
	assert(h->hash);
	assert(h->equals);
	
	unsigned long hash = h->hash(item);
	unsigned long chain = hash % h->size;
	
	list_foreach(h->table[chain], cur) {
		if (h->equals(cur, item))
			return false;
	}
	
	if (h->count + 1 > h->size) {
		size_t new_size = h->size * 2;
		list_t *temp = malloc(new_size * sizeof(list_t));
		if (temp != NULL)
			hash_set_rehash(h, temp, new_size);
		
		/*
		 * If the allocation fails, just use the same
		 * old table and try to rehash next time.
		 */
		chain = hash % h->size;
	}
	
	h->count++;
	list_append(item, &h->table[chain]);
	
	return true;
}

/** Search the hash set for a matching object and return it
 *
 * @param h    Hash set
 * @param item The item that should equal to the matched object
 *
 * @return Matching item on success, NULL if there is no such item.
 *
 */
link_t *hash_set_find(hash_set_t *h, const link_t *item)
{
	assert(h);
	assert(h->hash);
	assert(h->equals);
	
	unsigned long chain = h->hash(item) % h->size;
	
	list_foreach(h->table[chain], cur) {
		if (h->equals(cur, item))
			return cur;
	}
	
	return NULL;
}

/** Remove first matching object from the hash set and return it
 *
 * @param h    Hash set.
 * @param item The item that should be equal to the matched object
 *
 * @return The removed item or NULL if this is not found.
 *
 */
link_t *hash_set_remove(hash_set_t *h, const link_t *item)
{
	assert(h);
	assert(h->hash);
	assert(h->equals);
	
	link_t *cur = hash_set_find(h, item);
	if (cur) {
		list_remove(cur);
		
		h->count--;
		if (4 * h->count < h->size && h->size > HASH_SET_MIN_SIZE) {
			size_t new_size = h->size / 2;
			if (new_size < HASH_SET_MIN_SIZE)
				/* possible e.g. if init_size == HASH_SET_MIN_SIZE + 1 */
				new_size = HASH_SET_MIN_SIZE;
			
			list_t *temp = malloc(new_size * sizeof (list_t));
			if (temp != NULL)
				hash_set_rehash(h, temp, new_size);
		}
	}
	
	return cur;
}

/** Remove all elements for which the function returned non-zero
 *
 * The function can also destroy the element.
 *
 * @param h   Hash set.
 * @param f   Function to be applied.
 * @param arg Argument to be passed to the function.
 *
 */
void hash_set_remove_selected(hash_set_t *h, int (*f)(link_t *, void *),
    void *arg)
{
	assert(h);
	assert(h->table);
	
	for (size_t bucket = 0; bucket < h->size; bucket++) {
		link_t *prev = &h->table[bucket].head;
		link_t *cur;
		link_t *next;
		
		for (cur = h->table[bucket].head.next;
		    cur != &h->table[bucket].head;
		    cur = next) {
			next = cur->next;
			if (f(cur, arg)) {
				prev->next = next;
				next->prev = prev;
				h->count--;
			} else
				prev = cur;
		}
	}
	
	if (4 * h->count < h->size && h->size > HASH_SET_MIN_SIZE) {
		size_t new_size = h->size / 2;
		if (new_size < HASH_SET_MIN_SIZE)
			/* possible e.g. if init_size == HASH_SET_MIN_SIZE + 1 */
			new_size = HASH_SET_MIN_SIZE;
		
		list_t *temp = malloc(new_size * sizeof (list_t));
		if (temp != NULL)
			hash_set_rehash(h, temp, new_size);
	}
}

/** Apply function to all items in hash set
 *
 * @param h   Hash set.
 * @param f   Function to be applied.
 * @param arg Argument to be passed to the function.
 *
 */
void hash_set_apply(hash_set_t *h, void (*f)(link_t *, void *), void *arg)
{
	assert(h);
	assert(h->table);
	
	for (size_t bucket = 0; bucket < h->size; bucket++) {
		link_t *cur;
		link_t *next;
		
		for (cur = h->table[bucket].head.next;
		    cur != &h->table[bucket].head;
		    cur = next) {
			
			/*
			 * The next pointer must be stored prior to the functor
			 * call to allow using destructor as the functor (the
			 * free function could overwrite the cur->next pointer).
			 */
			next = cur->next;
			f(cur, arg);
		}
	}
}

/** Remove all elements from the set.
 *
 * The table is reallocated to the minimum size.
 *
 * @param h   Hash set
 * @param f   Function (destructor?) applied to all element. Can be NULL.
 * @param arg Argument to the destructor.
 *
 */
void hash_set_clear(hash_set_t *h, void (*f)(link_t *, void *), void *arg)
{
	assert(h);
	assert(h->table);
	
	for (size_t bucket = 0; bucket < h->size; bucket++) {
		link_t *cur;
		link_t *next;
		
		for (cur = h->table[bucket].head.next;
		    cur != &h->table[bucket].head;
		    cur = next) {
			next = cur->next;
			list_remove(cur);
			if (f != NULL)
				f(cur, arg);
		}
	}
	
	assert(h->size >= HASH_SET_MIN_SIZE);
	list_t *new_table =
	    realloc(h->table, HASH_SET_MIN_SIZE * sizeof(list_t));
	
	/* We are shrinking, therefore we shouldn't get NULL */
	assert(new_table);
	
	if (h->table != new_table) {
		/* Init the lists, pointers to itself are used in them */
		for (size_t bucket = 0; bucket < HASH_SET_MIN_SIZE; ++bucket)
			list_initialize(&new_table[bucket]);
		
		h->table = new_table;
	}
	
	h->count = 0;
	h->size = HASH_SET_MIN_SIZE;
}

/** Get hash set size
 *
 * @param hHash set
 *
 * @return Number of elements in the set.
 *
 */
size_t hash_set_count(const hash_set_t *h)
{
	assert(h);
	return h->count;
}

/** Check whether element is contained in the hash set
 *
 * @param h    Hash set
 * @param item Item that should be equal to the matched object
 *
 * @return True if the hash set contains equal object
 * @return False otherwise
 *
 */
int hash_set_contains(const hash_set_t *h, const link_t *item)
{
	/*
	 * The hash_set_find cannot accept constant hash set,
	 * because we can modify the returned element. But in
	 * this case we are using it safely.
	 */
	return hash_set_find((hash_set_t *) h, item) != NULL;
}

/** @}
 */
