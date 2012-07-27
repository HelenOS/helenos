/*
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup genericadt
 * @{
 */
/** @file
 */

#ifndef KERN_CONC_HASH_TABLE_H_
#define KERN_CONC_HASH_TABLE_H_

#include <stdint.h>
#include <adt/list.h>
#include <synch/rcu.h>
#include <macros.h>
#include <synch/workqueue.h>

typedef uintptr_t cht_ptr_t;

/** Concurrent hash table node link. */
typedef struct cht_link {
	/* Must be placed first. */
	rcu_item_t rcu_link;
	/** Link to the next item in the bucket including any marks. */
	cht_ptr_t link;
} cht_link_t;

/** Set of operations for a concurrent hash table. */
typedef struct cht_ops {
	size_t (*hash)(const cht_link_t *item);
	size_t (*key_hash)(void *key);
	bool (*equal)(const cht_link_t *item1, const cht_link_t *item2);
	bool (*key_equal)(void *key, const cht_link_t *item);
	void (*remove_callback)(cht_link_t *item);
} cht_ops_t;


typedef struct cht_buckets {
	size_t order;
	cht_ptr_t head[1];
} cht_buckets_t;

/** Concurrent hash table structure. */
typedef struct {
	cht_ops_t *op;
	
	size_t min_order;
	cht_buckets_t *b;
	cht_buckets_t *new_b;

	work_t resize_work;
	atomic_t resize_reqs;
	
	atomic_t item_cnt;
} cht_t;

#define cht_get_inst(item, type, member) \
	member_to_inst((item), type, member)


#define cht_read_lock()     rcu_read_lock()
#define cht_read_unlock()   rcu_read_unlock()

extern bool cht_create(cht_t *h, size_t init_size, size_t min_size, cht_ops_t *op);
extern void cht_destroy(cht_t *h);

extern cht_link_t *cht_find(cht_t *h, void *key);
extern cht_link_t *cht_find_lazy(cht_t *h, void *key);
extern cht_link_t *cht_find_next(cht_t *h, const cht_link_t *item);
extern cht_link_t *cht_find_next_lazy(cht_t *h, const cht_link_t *item);

extern void cht_insert(cht_t *h, cht_link_t *item);
extern bool cht_insert_unique(cht_t *h, cht_link_t *item);
extern size_t cht_remove_key(cht_t *h, void *key);
extern bool cht_remove_item(cht_t *h, cht_link_t *item);

#endif

/** @}
 */
