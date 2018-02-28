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

/**
 * @file
 * @brief Scalable resizable concurrent lock-free hash table.
 *
 * CHT is a concurrent hash table that is scalable resizable and lock-free.
 * resizable = the number of buckets of the table increases or decreases
 *     depending on the average number of elements per bucket (ie load)
 * scalable = accessing the table from more cpus increases performance
 *     almost linearly
 * lock-free = common operations never block; even if any of the operations
 *     is preempted or interrupted at any time, other operations will still
 *     make forward progress
 *
 * CHT is designed for read mostly scenarios. Performance degrades as the
 * fraction of updates (insert/remove) increases. Other data structures
 * significantly outperform CHT if the fraction of updates exceeds ~40%.
 *
 * CHT tolerates hardware exceptions and may be accessed from exception
 * handlers as long as the underlying RCU implementation is exception safe.
 *
 * @par Caveats
 *
 * 0) Never assume an item is still in the table.
 * The table may be accessed concurrently; therefore, other threads may
 * insert or remove an item at any time. Do not assume an item is still
 * in the table if cht_find() just returned it to you. Similarly, an
 * item may have already been inserted by the time cht_find() returns NULL.
 *
 * 1) Always use RCU read locks when searching the table.
 * Holding an RCU lock guarantees that an item found in the table remains
 * valid (eg is not freed) even if the item was removed from the table
 * in the meantime by another thread.
 *
 * 2) Never update values in place.
 * Do not update items in the table in place, ie directly. The changes
 * will not propagate to other readers (on other cpus) immediately or even
 * correctly. Some readers may then encounter items that have only some
 * of their fields changed or are completely inconsistent.
 *
 * Instead consider inserting an updated/changed copy of the item and
 * removing the original item. Or contact the maintainer to provide
 * you with a function that atomically replaces an item with a copy.
 *
 * 3) Use cht_insert_unique() instead of checking for duplicates with cht_find()
 * The following code is prone to race conditions:
 * @code
 * if (NULL == cht_find(&h, key)) {
 *     // If another thread inserts and item here, we'll insert a duplicate.
 *     cht_insert(&h, item);
 * }
 * @endcode
 * See cht_insert_unique() on how to correctly fix this.
 *
 *
 * @par Semantics
 *
 * Lazy readers = cht_find_lazy(), cht_find_next_lazy()
 * Readers = lazy readers, cht_find(), cht_find_next()
 * Updates = cht_insert(), cht_insert_unique(), cht_remove_key(),
 *     cht_remove_item()
 *
 * Readers (but not lazy readers) are guaranteed to see the effects
 * of @e completed updates. In other words, if cht_find() is invoked
 * after a cht_insert() @e returned eg on another cpu, cht_find() is
 * guaranteed to see the inserted item.
 *
 * Similarly, updates see the effects of @e completed updates. For example,
 * issuing cht_remove() after a cht_insert() for that key returned (even
 * on another cpu) is guaranteed to remove the inserted item.
 *
 * Reading or updating the table concurrently with other updates
 * always returns consistent data and never corrupts the table.
 * However the effects of concurrent updates may or may not be
 * visible to all other concurrent readers or updaters. Eg, not
 * all readers may see that an item has already been inserted
 * if cht_insert() has not yet returned.
 *
 * Lazy readers are guaranteed to eventually see updates but it
 * may take some time (possibly milliseconds) after the update
 * completes for the change to propagate to lazy readers on all
 * cpus.
 *
 * @par Implementation
 *
 * Collisions in CHT are resolved with chaining. The number of buckets
 * is always a power of 2. Each bucket is represented with a single linked
 * lock-free list [1]. Items in buckets are sorted by their mixed hashes
 * in ascending order. All buckets are terminated with a single global
 * sentinel node whose mixed hash value is the greatest possible.
 *
 * CHT with 2^k buckets uses the k most significant bits of a hash value
 * to determine the bucket number where an item is to be stored. To
 * avoid storing all items in a single bucket if the user supplied
 * hash function does not produce uniform hashes, hash values are
 * mixed first so that the top bits of a mixed hash change even if hash
 * values differ only in the least significant bits. The mixed hash
 * values are cached in cht_link.hash (which is overwritten once the
 * item is scheduled for removal via rcu_call).
 *
 * A new item is inserted before all other existing items in the bucket
 * with the same hash value as the newly inserted item (a la the original
 * lock-free list [2]). Placing new items at the start of a same-hash
 * sequence of items (eg duplicates) allows us to easily check for duplicates
 * in cht_insert_unique(). The function can first check that there are
 * no duplicates of the newly inserted item amongst the items with the
 * same hash as the new item. If there were no duplicates the new item
 * is linked before the same-hash items. Inserting a duplicate while
 * the function is checking for duplicates is detected as a change of
 * the link to the first checked same-hash item (and the search for
 * duplicates can be restarted).
 *
 * @par Table resize algorithm
 *
 * Table resize is based on [3] and [5]. First, a new bucket head array
 * is allocated and initialized. Second, old bucket heads are moved
 * to the new bucket head array with the protocol mentioned in [5].
 * At this point updaters start using the new bucket heads. Third,
 * buckets are split (or joined) so that the table can make use of
 * the extra bucket head slots in the new array (or stop wasting space
 * with the unnecessary extra slots in the old array). Splitting
 * or joining buckets employs a custom protocol. Last, the new array
 * replaces the original bucket array.
 *
 * A single background work item (of the system work queue) guides
 * resizing of the table. If an updater detects that the bucket it
 * is about to access is undergoing a resize (ie its head is moving
 * or it needs to be split/joined), it helps out and completes the
 * head move or the bucket split/join.
 *
 * The table always grows or shrinks by a factor of 2. Because items
 * are assigned a bucket based on the top k bits of their mixed hash
 * values, when growing the table each bucket is split into two buckets
 * and all items of the two new buckets come from the single bucket in the
 * original table. Ie items from separate buckets in the original table
 * never intermix in the new buckets. Moreover
 * since the buckets are sorted by their mixed hash values the items
 * at the beginning of the old bucket will end up in the first new
 * bucket while all the remaining items of the old bucket will end up
 * in the second new bucket. Therefore, there is a single point where
 * to split the linked list of the old bucket into two correctly sorted
 * linked lists of the new buckets:
 *                            .- bucket split
 *                            |
 *             <-- first -->  v  <-- second -->
 *   [old] --> [00b] -> [01b] -> [10b] -> [11b] -> sentinel
 *              ^                 ^
 *   [new0] -- -+                 |
 *   [new1] -- -- -- -- -- -- -- -+
 *
 * Resize in greater detail:
 *
 * a) First, a resizer (a single background system work queue item
 * in charge of resizing the table) allocates and initializes a new
 * bucket head array. New bucket heads are pointed to the sentinel
 * and marked Invalid (in the lower order bits of the pointer to the
 * next item, ie the sentinel in this case):
 *
 *   [old, N] --> [00b] -> [01b] -> [10b] -> [11b] -> sentinel
 *                                                    ^ ^
 *   [new0, Inv] -------------------------------------+ |
 *   [new1, Inv] ---------------------------------------+
 *
 *
 * b) Second, the resizer starts moving old bucket heads with the following
 * lock-free protocol (from [5]) where cas(variable, expected_val, new_val)
 * is short for compare-and-swap:
 *
 *   old head     new0 head      transition to next state
 *   --------     ---------      ------------------------
 *   addr, N      sentinel, Inv  cas(old, (addr, N), (addr, Const))
 *                               .. mark the old head as immutable, so that
 *                                  updaters do not relink it to other nodes
 *                                  until the head move is done.
 *   addr, Const  sentinel, Inv  cas(new0, (sentinel, Inv), (addr, N))
 *                               .. move the address to the new head and mark
 *                                  the new head normal so updaters can start
 *                                  using it.
 *   addr, Const  addr, N        cas(old, (addr, Const), (addr, Inv))
 *                               .. mark the old head Invalid to signify
 *                                  the head move is done.
 *   addr, Inv    addr, N
 *
 * Notice that concurrent updaters may step in at any point and correctly
 * complete the head move without disrupting the resizer. At worst, the
 * resizer or other concurrent updaters will attempt a number of CAS() that
 * will correctly fail.
 *
 *   [old, Inv] -> [00b] -> [01b] -> [10b] -> [11b] -> sentinel
 *                 ^                                   ^
 *   [new0, N] ----+                                   |
 *   [new1, Inv] --------------------------------------+
 *
 *
 * c) Third, buckets are split if the table is growing; or joined if
 * shrinking (by the resizer or updaters depending on whoever accesses
 * the bucket first). See split_bucket() and join_buckets() for details.
 *
 *  1) Mark the last item of new0 with JOIN_FOLLOWS:
 *   [old, Inv] -> [00b] -> [01b, JF] -> [10b] -> [11b] -> sentinel
 *                 ^                                       ^
 *   [new0, N] ----+                                       |
 *   [new1, Inv] ------------------------------------------+
 *
 *  2) Mark the first item of new1 with JOIN_NODE:
 *   [old, Inv] -> [00b] -> [01b, JF] -> [10b, JN] -> [11b] -> sentinel
 *                 ^                                           ^
 *   [new0, N] ----+                                           |
 *   [new1, Inv] ----------------------------------------------+
 *
 *  3) Point new1 to the join-node and mark new1 NORMAL.
 *   [old, Inv] -> [00b] -> [01b, JF] -> [10b, JN] -> [11b] -> sentinel
 *                 ^                     ^
 *   [new0, N] ----+                     |
 *   [new1, N] --------------------------+
 *
 *
 * d) Fourth, the resizer cleans up extra marks added during bucket
 * splits/joins but only when it is sure all updaters are accessing
 * the table via the new bucket heads only (ie it is certain there
 * are no delayed updaters unaware of the resize and accessing the
 * table via the old bucket head).
 *
 *   [old, Inv] ---+
 *                 v
 *   [new0, N] --> [00b] -> [01b, N] ---+
 *                                      v
 *   [new1, N] --> [10b, N] -> [11b] -> sentinel
 *
 *
 * e) Last, the resizer publishes the new bucket head array for everyone
 * to see and use. This signals the end of the resize and the old bucket
 * array is freed.
 *
 *
 * To understand details of how the table is resized, read [1, 3, 5]
 * and comments in join_buckets(), split_bucket().
 *
 *
 * [1] High performance dynamic lock-free hash tables and list-based sets,
 *     Michael, 2002
 *     http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 * [2] Lock-free linked lists using compare-and-swap,
 *     Valois, 1995
 *     http://people.csail.mit.edu/bushl2/rpi/portfolio/lockfree-grape/documents/lock-free-linked-lists.pdf
 * [3] Resizable, scalable, concurrent hash tables via relativistic programming,
 *     Triplett, 2011
 *     http://www.usenix.org/event/atc11/tech/final_files/Triplett.pdf
 * [4] Split-ordered Lists: Lock-free Extensible Hash Tables,
 *     Shavit, 2006
 *     http://www.cs.ucf.edu/~dcm/Teaching/COT4810-Spring2011/Literature/SplitOrderedLists.pdf
 * [5] Towards a Scalable Non-blocking Coding Style,
 *     Click, 2008
 *     http://www.azulsystems.com/events/javaone_2008/2008_CodingNonBlock.pdf
 */


#include <adt/cht.h>
#include <adt/hash.h>
#include <assert.h>
#include <mm/slab.h>
#include <arch/barrier.h>
#include <compiler/barrier.h>
#include <atomic.h>
#include <synch/rcu.h>

#ifdef CONFIG_DEBUG
/* Do not enclose in parentheses. */
#define DBG(x) x
#else
#define DBG(x)
#endif

/* Logarithm of the min bucket count. Must be at least 3. 2^6 == 64 buckets. */
#define CHT_MIN_ORDER 6
/* Logarithm of the max bucket count. */
#define CHT_MAX_ORDER (8 * sizeof(size_t))
/* Minimum number of hash table buckets. */
#define CHT_MIN_BUCKET_CNT (1 << CHT_MIN_ORDER)
/* Does not have to be a power of 2. */
#define CHT_MAX_LOAD 2

typedef cht_ptr_t marked_ptr_t;
typedef bool (*equal_pred_t)(void *arg, const cht_link_t *item);

/** The following mark items and bucket heads.
 *
 * They are stored in the two low order bits of the next item pointers.
 * Some marks may be combined. Some marks share the same binary value and
 * are distinguished only by context (eg bucket head vs an ordinary item),
 * in particular by walk_mode_t.
 */
typedef enum mark {
	/** Normal non-deleted item or a valid bucket head. */
	N_NORMAL = 0,
	/** Logically deleted item that might have already been unlinked.
	 *
	 * May be combined with N_JOIN and N_JOIN_FOLLOWS. Applicable only
	 * to items; never to bucket heads.
	 *
	 * Once marked deleted an item remains marked deleted.
	 */
	N_DELETED = 1,
	/** Immutable bucket head.
	 *
	 * The bucket is being moved or joined with another and its (old) head
	 * must not be modified.
	 *
	 * May be combined with N_INVALID. Applicable only to old bucket heads,
	 * ie cht_t.b and not cht_t.new_b.
	 */
	N_CONST = 1,
	/** Invalid bucket head. The bucket head must not be modified.
	 *
	 * Old bucket heads (ie cht_t.b) are marked invalid if they have
	 * already been moved to cht_t.new_b or if the bucket had already
	 * been merged with another when shrinking the table. New bucket
	 * heads (ie cht_t.new_b) are marked invalid if the old bucket had
	 * not yet been moved or if an old bucket had not yet been split
	 * when growing the table.
	 */
	N_INVALID = 3,
	/** The item is a join node, ie joining two buckets
	 *
	 * A join node is either the first node of the second part of
	 * a bucket to be split; or it is the first node of the bucket
	 * to be merged into/appended to/joined with another bucket.
	 *
	 * May be combined with N_DELETED. Applicable only to items, never
	 * to bucket heads.
	 *
	 * Join nodes are referred to from two different buckets and may,
	 * therefore, not be safely/atomically unlinked from both buckets.
	 * As a result join nodes are not unlinked but rather just marked
	 * deleted. Once resize completes join nodes marked deleted are
	 * garbage collected.
	 */
	N_JOIN = 2,
	/** The next node is a join node and will soon be marked so.
	 *
	 * A join-follows node is the last node of the first part of bucket
	 * that is to be split, ie it is the last node that will remain
	 * in the same bucket after splitting it.
	 *
	 * May be combined with N_DELETED. Applicable to items as well
	 * as to bucket heads of the bucket to be split (but only in cht_t.new_b).
	 */
	N_JOIN_FOLLOWS = 2,
	/** Bit mask to filter out the address to the next item from the next ptr. */
	N_MARK_MASK = 3
} mark_t;

/** Determines */
typedef enum walk_mode {
	/** The table is not resizing. */
	WM_NORMAL = 4,
	/** The table is undergoing a resize. Join nodes may be encountered. */
	WM_LEAVE_JOIN,
	/** The table is growing. A join-follows node may be encountered. */
	WM_MOVE_JOIN_FOLLOWS
} walk_mode_t;

/** Bucket position window. */
typedef struct wnd {
	/** Pointer to cur's predecessor. */
	marked_ptr_t *ppred;
	/** Current item. */
	cht_link_t *cur;
	/** Last encountered item. Deleted or not. */
	cht_link_t *last;
} wnd_t;


/* Sentinel node used by all buckets. Stores the greatest possible hash value.*/
static const cht_link_t sentinel = {
	/* NULL and N_NORMAL */
	.link = 0 | N_NORMAL,
	.hash = -1
};


static size_t size_to_order(size_t bucket_cnt, size_t min_order);
static cht_buckets_t *alloc_buckets(size_t order, bool set_invalid,
	bool can_block);
static inline cht_link_t *find_lazy(cht_t *h, void *key);
static cht_link_t *search_bucket(cht_t *h, marked_ptr_t head, void *key,
	size_t search_hash);
static cht_link_t *find_resizing(cht_t *h, void *key, size_t hash,
	marked_ptr_t old_head, size_t old_idx);
static bool insert_impl(cht_t *h, cht_link_t *item, cht_link_t **dup_item);
static bool insert_at(cht_link_t *item, const wnd_t *wnd, walk_mode_t walk_mode,
	bool *resizing);
static bool has_duplicate(cht_t *h, const cht_link_t *item, size_t hash,
	cht_link_t *cur, cht_link_t **dup_item);
static cht_link_t *find_duplicate(cht_t *h, const cht_link_t *item, size_t hash,
	cht_link_t *start);
static bool remove_pred(cht_t *h, size_t hash, equal_pred_t pred, void *pred_arg);
static bool delete_at(cht_t *h, wnd_t *wnd, walk_mode_t walk_mode,
	bool *deleted_but_gc, bool *resizing);
static bool mark_deleted(cht_link_t *cur, walk_mode_t walk_mode, bool *resizing);
static bool unlink_from_pred(wnd_t *wnd, walk_mode_t walk_mode, bool *resizing);
static bool find_wnd_and_gc_pred(cht_t *h, size_t hash, walk_mode_t walk_mode,
	equal_pred_t pred, void *pred_arg, wnd_t *wnd, bool *resizing);
static bool find_wnd_and_gc(cht_t *h, size_t hash, walk_mode_t walk_mode,
	wnd_t *wnd, bool *resizing);
static bool gc_deleted_node(cht_t *h, walk_mode_t walk_mode, wnd_t *wnd,
	bool *resizing);
static bool join_completed(cht_t *h, const wnd_t *wnd);
static void upd_resizing_head(cht_t *h, size_t hash, marked_ptr_t **phead,
	bool *join_finishing,  walk_mode_t *walk_mode);
static void item_removed(cht_t *h);
static void item_inserted(cht_t *h);
static void free_later(cht_t *h, cht_link_t *item);
static void help_head_move(marked_ptr_t *psrc_head, marked_ptr_t *pdest_head);
static void start_head_move(marked_ptr_t *psrc_head);
static void mark_const(marked_ptr_t *psrc_head);
static void complete_head_move(marked_ptr_t *psrc_head, marked_ptr_t *pdest_head);
static void split_bucket(cht_t *h, marked_ptr_t *psrc_head,
	marked_ptr_t *pdest_head, size_t split_hash);
static void mark_join_follows(cht_t *h, marked_ptr_t *psrc_head,
	size_t split_hash, wnd_t *wnd);
static void mark_join_node(cht_link_t *join_node);
static void join_buckets(cht_t *h, marked_ptr_t *psrc_head,
	marked_ptr_t *pdest_head, size_t split_hash);
static void link_to_join_node(cht_t *h, marked_ptr_t *pdest_head,
	cht_link_t *join_node, size_t split_hash);
static void resize_table(work_t *arg);
static void grow_table(cht_t *h);
static void shrink_table(cht_t *h);
static void cleanup_join_node(cht_t *h, marked_ptr_t *new_head);
static void clear_join_and_gc(cht_t *h, cht_link_t *join_node,
	marked_ptr_t *new_head);
static void cleanup_join_follows(cht_t *h, marked_ptr_t *new_head);
static marked_ptr_t make_link(const cht_link_t *next, mark_t mark);
static cht_link_t * get_next(marked_ptr_t link);
static mark_t get_mark(marked_ptr_t link);
static void next_wnd(wnd_t *wnd);
static bool same_node_pred(void *node, const cht_link_t *item2);
static size_t calc_key_hash(cht_t *h, void *key);
static size_t node_hash(cht_t *h, const cht_link_t *item);
static size_t calc_node_hash(cht_t *h, const cht_link_t *item);
static void memoize_node_hash(cht_t *h, cht_link_t *item);
static size_t calc_split_hash(size_t split_idx, size_t order);
static size_t calc_bucket_idx(size_t hash, size_t order);
static size_t grow_to_split_idx(size_t old_idx);
static size_t grow_idx(size_t idx);
static size_t shrink_idx(size_t idx);
static marked_ptr_t cas_link(marked_ptr_t *link, const cht_link_t *cur_next,
	mark_t cur_mark, const cht_link_t *new_next, mark_t new_mark);
static marked_ptr_t _cas_link(marked_ptr_t *link, marked_ptr_t cur,
	marked_ptr_t new);
static void cas_order_barrier(void);

static void dummy_remove_callback(cht_link_t *item)
{
	/* empty */
}

/** Creates a concurrent hash table.
 *
 * @param h         Valid pointer to a cht_t instance.
 * @param op        Item specific operations. All operations are compulsory.
 * @return True if successfully created the table. False otherwise.
 */
bool cht_create_simple(cht_t *h, cht_ops_t *op)
{
	return cht_create(h, 0, 0, 0, false, op);
}

/** Creates a concurrent hash table.
 *
 * @param h         Valid pointer to a cht_t instance.
 * @param init_size The initial number of buckets the table should contain.
 *                  The table may be shrunk below this value if deemed necessary.
 *                  Uses the default value if 0.
 * @param min_size  Minimum number of buckets that the table should contain.
 *                  The number of buckets never drops below this value,
 *                  although it may be rounded up internally as appropriate.
 *                  Uses the default value if 0.
 * @param max_load  Maximum average number of items per bucket that allowed
 *                  before the table grows.
 * @param can_block If true creating the table blocks until enough memory
 *                  is available (possibly indefinitely). Otherwise,
 *                  table creation does not block and returns immediately
 *                  even if not enough memory is available.
 * @param op        Item specific operations. All operations are compulsory.
 * @return True if successfully created the table. False otherwise.
 */
bool cht_create(cht_t *h, size_t init_size, size_t min_size, size_t max_load,
	bool can_block, cht_ops_t *op)
{
	assert(h);
	assert(op && op->hash && op->key_hash && op->equal && op->key_equal);
	/* Memoized hashes are stored in the rcu_link.func function pointer. */
	static_assert(sizeof(size_t) == sizeof(rcu_func_t), "");
	assert(sentinel.hash == (uintptr_t)sentinel.rcu_link.func);

	/* All operations are compulsory. */
	if (!op || !op->hash || !op->key_hash || !op->equal || !op->key_equal)
		return false;

	size_t min_order = size_to_order(min_size, CHT_MIN_ORDER);
	size_t order = size_to_order(init_size, min_order);

	h->b = alloc_buckets(order, false, can_block);

	if (!h->b)
		return false;

	h->max_load = (max_load == 0) ? CHT_MAX_LOAD : max_load;
	h->min_order = min_order;
	h->new_b = NULL;
	h->op = op;
	atomic_set(&h->item_cnt, 0);
	atomic_set(&h->resize_reqs, 0);

	if (NULL == op->remove_callback) {
		h->op->remove_callback = dummy_remove_callback;
	}

	/*
	 * Cached item hashes are stored in item->rcu_link.func. Once the item
	 * is deleted rcu_link.func will contain the value of invalid_hash.
	 */
	h->invalid_hash = (uintptr_t)h->op->remove_callback;

	/* Ensure the initialization takes place before we start using the table. */
	write_barrier();

	return true;
}

/** Allocates and initializes 2^order buckets.
 *
 * All bucket heads are initialized to point to the sentinel node.
 *
 * @param order       The number of buckets to allocate is 2^order.
 * @param set_invalid Bucket heads are marked invalid if true; otherwise
 *                    they are marked N_NORMAL.
 * @param can_block   If true memory allocation blocks until enough memory
 *                    is available (possibly indefinitely). Otherwise,
 *                    memory allocation does not block.
 * @return Newly allocated and initialized buckets or NULL if not enough memory.
 */
static cht_buckets_t *alloc_buckets(size_t order, bool set_invalid, bool can_block)
{
	size_t bucket_cnt = (1 << order);
	size_t bytes =
		sizeof(cht_buckets_t) + (bucket_cnt - 1) * sizeof(marked_ptr_t);
	cht_buckets_t *b = malloc(bytes, can_block ? 0 : FRAME_ATOMIC);

	if (!b)
		return NULL;

	b->order = order;

	marked_ptr_t head_link = set_invalid
		? make_link(&sentinel, N_INVALID)
		: make_link(&sentinel, N_NORMAL);

	for (size_t i = 0; i < bucket_cnt; ++i) {
		b->head[i] = head_link;
	}

	return b;
}

/** Returns the smallest k such that bucket_cnt <= 2^k and min_order <= k.*/
static size_t size_to_order(size_t bucket_cnt, size_t min_order)
{
	size_t order = min_order;

	/* Find a power of two such that bucket_cnt <= 2^order */
	do {
		if (bucket_cnt <= ((size_t)1 << order))
			return order;

		++order;
	} while (order < CHT_MAX_ORDER);

	return order;
}

/** Destroys a CHT successfully created via cht_create().
 *
 * Waits for all outstanding concurrent operations to complete and
 * frees internal allocated resources. The table is however not cleared
 * and items already present in the table (if any) are leaked.
 */
void cht_destroy(cht_t *h)
{
	cht_destroy_unsafe(h);

	/* You must clear the table of items. Otherwise cht_destroy will leak. */
	assert(atomic_get(&h->item_cnt) == 0);
}

/** Destroys a successfully created CHT but does no error checking. */
void cht_destroy_unsafe(cht_t *h)
{
	/* Wait for resize to complete. */
	while (0 < atomic_get(&h->resize_reqs)) {
		rcu_barrier();
	}

	/* Wait for all remove_callback()s to complete. */
	rcu_barrier();

	free(h->b);
	h->b = NULL;
}

/** Returns the first item equal to the search key or NULL if not found.
 *
 * The call must be enclosed in a rcu_read_lock() unlock() pair. The
 * returned item is guaranteed to be allocated until rcu_read_unlock()
 * although the item may be concurrently removed from the table by another
 * cpu.
 *
 * Further items matching the key may be retrieved via cht_find_next().
 *
 * cht_find() sees the effects of any completed cht_remove(), cht_insert().
 * If a concurrent remove or insert had not yet completed cht_find() may
 * or may not see the effects of it (eg it may find an item being removed).
 *
 * @param h   CHT to operate on.
 * @param key Search key as defined by cht_ops_t.key_equal() and .key_hash().
 * @return First item equal to the key or NULL if such an item does not exist.
 */
cht_link_t *cht_find(cht_t *h, void *key)
{
	/* Make the most recent changes to the table visible. */
	read_barrier();
	return cht_find_lazy(h, key);
}

/** Returns the first item equal to the search key or NULL if not found.
 *
 * Unlike cht_find(), cht_find_lazy() may not see the effects of
 * cht_remove() or cht_insert() even though they have already completed.
 * It may take a couple of milliseconds for those changes to propagate
 * and become visible to cht_find_lazy(). On the other hand, cht_find_lazy()
 * operates a bit faster than cht_find().
 *
 * See cht_find() for more details.
 */
cht_link_t *cht_find_lazy(cht_t *h, void *key)
{
	return find_lazy(h, key);
}

/** Finds the first item equal to the search key. */
static inline cht_link_t *find_lazy(cht_t *h, void *key)
{
	assert(h);
	/* See docs to cht_find() and cht_find_lazy(). */
	assert(rcu_read_locked());

	size_t hash = calc_key_hash(h, key);

	cht_buckets_t *b = rcu_access(h->b);
	size_t idx = calc_bucket_idx(hash, b->order);
	/*
	 * No need for access_once. b->head[idx] will point to an allocated node
	 * even if marked invalid until we exit rcu read section.
	 */
	marked_ptr_t head = b->head[idx];

	/* Undergoing a resize - take the slow path. */
	if (N_INVALID == get_mark(head))
		return find_resizing(h, key, hash, head, idx);

	return search_bucket(h, head, key, hash);
}

/** Returns the next item matching \a item.
 *
 * Must be enclosed in a rcu_read_lock()/unlock() pair. Effects of
 * completed cht_remove(), cht_insert() are guaranteed to be visible
 * to cht_find_next().
 *
 * See cht_find() for more details.
 */
cht_link_t *cht_find_next(cht_t *h, const cht_link_t *item)
{
	/* Make the most recent changes to the table visible. */
	read_barrier();
	return cht_find_next_lazy(h, item);
}

/** Returns the next item matching \a item.
 *
 * Must be enclosed in a rcu_read_lock()/unlock() pair. Effects of
 * completed cht_remove(), cht_insert() may or may not be visible
 * to cht_find_next_lazy().
 *
 * See cht_find_lazy() for more details.
 */
cht_link_t *cht_find_next_lazy(cht_t *h, const cht_link_t *item)
{
	assert(h);
	assert(rcu_read_locked());
	assert(item);

	return find_duplicate(h, item, calc_node_hash(h, item), get_next(item->link));
}

/** Searches the bucket at head for key using search_hash. */
static inline cht_link_t *search_bucket(cht_t *h, marked_ptr_t head, void *key,
	size_t search_hash)
{
	/*
	 * It is safe to access nodes even outside of this bucket (eg when
	 * splitting the bucket). The resizer makes sure that any node we
	 * may find by following the next pointers is allocated.
	 */

	cht_link_t *cur = NULL;
	marked_ptr_t prev = head;

try_again:
	/* Filter out items with different hashes. */
	do {
		cur = get_next(prev);
		assert(cur);
		prev = cur->link;
	} while (node_hash(h, cur) < search_hash);

	/*
	 * Only search for an item with an equal key if cur is not the sentinel
	 * node or a node with a different hash.
	 */
	while (node_hash(h, cur) == search_hash) {
		if (h->op->key_equal(key, cur)) {
			if (!(N_DELETED & get_mark(cur->link)))
				return cur;
		}

		cur = get_next(cur->link);
		assert(cur);
	}

	/*
	 * In the unlikely case that we have encountered a node whose cached
	 * hash has been overwritten due to a pending rcu_call for it, skip
	 * the node and try again.
	 */
	if (node_hash(h, cur) == h->invalid_hash) {
		prev = cur->link;
		goto try_again;
	}

	return NULL;
}

/** Searches for the key while the table is undergoing a resize. */
static cht_link_t *find_resizing(cht_t *h, void *key, size_t hash,
	marked_ptr_t old_head, size_t old_idx)
{
	assert(N_INVALID == get_mark(old_head));
	assert(h->new_b);

	size_t new_idx = calc_bucket_idx(hash, h->new_b->order);
	marked_ptr_t new_head = h->new_b->head[new_idx];
	marked_ptr_t search_head = new_head;

	/* Growing. */
	if (h->b->order < h->new_b->order) {
		/*
		 * Old bucket head is invalid, so it must have been already
		 * moved. Make the new head visible if still not visible, ie
		 * invalid.
		 */
		if (N_INVALID == get_mark(new_head)) {
			/*
			 * We should be searching a newly added bucket but the old
			 * moved bucket has not yet been split (its marked invalid)
			 * or we have not yet seen the split.
			 */
			if (grow_idx(old_idx) != new_idx) {
				/*
				 * Search the moved bucket. It is guaranteed to contain
				 * items of the newly added bucket that were present
				 * before the moved bucket was split.
				 */
				new_head = h->new_b->head[grow_idx(old_idx)];
			}

			/* new_head is now the moved bucket, either valid or invalid. */

			/*
			 * The old bucket was definitely moved to new_head but the
			 * change of new_head had not yet propagated to this cpu.
			 */
			if (N_INVALID == get_mark(new_head)) {
				/*
				 * We could issue a read_barrier() and make the now valid
				 * moved bucket head new_head visible, but instead fall back
				 * on using the old bucket. Although the old bucket head is
				 * invalid, it points to a node that is allocated and in the
				 * right bucket. Before the node can be freed, it must be
				 * unlinked from the head (or another item after that item
				 * modified the new_head) and a grace period must elapse.
				 * As a result had the node been already freed the grace
				 * period preceeding the free() would make the unlink and
				 * any changes to new_head visible. Therefore, it is safe
				 * to use the node pointed to from the old bucket head.
				 */

				search_head = old_head;
			} else {
				search_head = new_head;
			}
		}

		return search_bucket(h, search_head, key, hash);
	} else if (h->b->order > h->new_b->order) {
		/* Shrinking. */

		/* Index of the bucket in the old table that was moved. */
		size_t move_src_idx = grow_idx(new_idx);
		marked_ptr_t moved_old_head = h->b->head[move_src_idx];

		/*
		 * h->b->head[move_src_idx] had already been moved to new_head
		 * but the change to new_head had not yet propagated to us.
		 */
		if (N_INVALID == get_mark(new_head)) {
			/*
			 * new_head is definitely valid and we could make it visible
			 * to this cpu with a read_barrier(). Instead, use the bucket
			 * in the old table that was moved even though it is now marked
			 * as invalid. The node it points to must be allocated because
			 * a grace period would have to elapse before it could be freed;
			 * and the grace period would make the now valid new_head
			 * visible to all cpus.
			 *
			 * Note that move_src_idx may not be the same as old_idx.
			 * If move_src_idx != old_idx then old_idx is the bucket
			 * in the old table that is not moved but instead it is
			 * appended to the moved bucket, ie it is added at the tail
			 * of new_head. In that case an invalid old_head notes that
			 * it had already been merged into (the moved) new_head.
			 * We will try to search that bucket first because it
			 * may contain some newly added nodes after the bucket
			 * join. Moreover, the bucket joining link may already be
			 * visible even if new_head is not. Therefore, if we're
			 * lucky we'll find the item via moved_old_head. In any
			 * case, we'll retry in proper old_head if not found.
			 */
			search_head = moved_old_head;
		}

		cht_link_t *ret = search_bucket(h, search_head, key, hash);

		if (ret)
			return ret;
		/*
		 * Bucket old_head was already joined with moved_old_head
		 * in the new table but we have not yet seen change of the
		 * joining link (or the item is not in the table).
		 */
		if (move_src_idx != old_idx && get_next(old_head) != &sentinel) {
			/*
			 * Note that old_head (the bucket to be merged into new_head)
			 * points to an allocated join node (if non-null) even if marked
			 * invalid. Before the resizer lets join nodes to be unlinked
			 * (and freed) it sets old_head to NULL and waits for a grace period.
			 * So either the invalid old_head points to join node; or old_head
			 * is null and we would have seen a completed bucket join while
			 * traversing search_head.
			 */
			assert(N_JOIN & get_mark(get_next(old_head)->link));
			return search_bucket(h, old_head, key, hash);
		}

		return NULL;
	} else {
		/*
		 * Resize is almost done. The resizer is waiting to make
		 * sure all cpus see that the new table replaced the old one.
		 */
		assert(h->b->order == h->new_b->order);
		/*
		 * The resizer must ensure all new bucket heads are visible before
		 * replacing the old table.
		 */
		assert(N_NORMAL == get_mark(new_head));
		return search_bucket(h, new_head, key, hash);
	}
}

/** Inserts an item. Succeeds even if an equal item is already present. */
void cht_insert(cht_t *h, cht_link_t *item)
{
	insert_impl(h, item, NULL);
}

/** Inserts a unique item. Returns false if an equal item was already present.
 *
 * Use this function to atomically check if an equal/duplicate item had
 * not yet been inserted into the table and to insert this item into the
 * table.
 *
 * The following is @e NOT thread-safe, so do not use:
 * @code
 * if (!cht_find(h, key)) {
 *     // A concurrent insert here may go unnoticed by cht_find() above.
 *     item = malloc(..);
 *     cht_insert(h, item);
 *     // Now we may have two items with equal search keys.
 * }
 * @endcode
 *
 * Replace such code with:
 * @code
 * item = malloc(..);
 * if (!cht_insert_unique(h, item, &dup_item)) {
 *     // Whoops, someone beat us to it - an equal item 'dup_item'
 *     // had already been inserted.
 *     free(item);
 * } else {
 *     // Successfully inserted the item and we are guaranteed that
 *     // there are no other equal items.
 * }
 * @endcode
 *
 */
bool cht_insert_unique(cht_t *h, cht_link_t *item, cht_link_t **dup_item)
{
	assert(rcu_read_locked());
	assert(dup_item);
	return insert_impl(h, item, dup_item);
}

/** Inserts the item into the table and checks for duplicates if dup_item. */
static bool insert_impl(cht_t *h, cht_link_t *item, cht_link_t **dup_item)
{
	rcu_read_lock();

	cht_buckets_t *b = rcu_access(h->b);
	memoize_node_hash(h, item);
	size_t hash = node_hash(h, item);
	size_t idx = calc_bucket_idx(hash, b->order);
	marked_ptr_t *phead = &b->head[idx];

	bool resizing = false;
	bool inserted = false;

	do {
		walk_mode_t walk_mode = WM_NORMAL;
		bool join_finishing;

		resizing = resizing || (N_NORMAL != get_mark(*phead));

		/* The table is resizing. Get the correct bucket head. */
		if (resizing) {
			upd_resizing_head(h, hash, &phead, &join_finishing, &walk_mode);
		}

		wnd_t wnd = {
			.ppred = phead,
			.cur = get_next(*phead),
			.last = NULL
		};

		if (!find_wnd_and_gc(h, hash, walk_mode, &wnd, &resizing)) {
			/* Could not GC a node; or detected an unexpected resize. */
			continue;
		}

		if (dup_item && has_duplicate(h, item, hash, wnd.cur, dup_item)) {
			rcu_read_unlock();
			return false;
		}

		inserted = insert_at(item, &wnd, walk_mode, &resizing);
	} while (!inserted);

	rcu_read_unlock();

	item_inserted(h);
	return true;
}

/** Inserts item between wnd.ppred and wnd.cur.
 *
 * @param item      Item to link to wnd.ppred and wnd.cur.
 * @param wnd       The item will be inserted before wnd.cur. Wnd.ppred
 *                  must be N_NORMAL.
 * @param walk_mode
 * @param resizing  Set to true only if the table is undergoing resize
 *         and it was not expected (ie walk_mode == WM_NORMAL).
 * @return True if the item was successfully linked to wnd.ppred. False
 *         if whole insert operation must be retried because the predecessor
 *         of wnd.cur has changed.
 */
inline static bool insert_at(cht_link_t *item, const wnd_t *wnd,
	walk_mode_t walk_mode, bool *resizing)
{
	marked_ptr_t ret;

	if (walk_mode == WM_NORMAL) {
		item->link = make_link(wnd->cur, N_NORMAL);
		/* Initialize the item before adding it to a bucket. */
		memory_barrier();

		/* Link a clean/normal predecessor to the item. */
		ret = cas_link(wnd->ppred, wnd->cur, N_NORMAL, item, N_NORMAL);

		if (ret == make_link(wnd->cur, N_NORMAL)) {
			return true;
		} else {
			/* This includes an invalid head but not a const head. */
			*resizing = ((N_JOIN_FOLLOWS | N_JOIN) & get_mark(ret));
			return false;
		}
	} else if (walk_mode == WM_MOVE_JOIN_FOLLOWS) {
		/* Move JOIN_FOLLOWS mark but filter out the DELETED mark. */
		mark_t jf_mark = get_mark(*wnd->ppred) & N_JOIN_FOLLOWS;
		item->link = make_link(wnd->cur, jf_mark);
		/* Initialize the item before adding it to a bucket. */
		memory_barrier();

		/* Link the not-deleted predecessor to the item. Move its JF mark. */
		ret = cas_link(wnd->ppred, wnd->cur, jf_mark, item, N_NORMAL);

		return ret == make_link(wnd->cur, jf_mark);
	} else {
		assert(walk_mode == WM_LEAVE_JOIN);

		item->link = make_link(wnd->cur, N_NORMAL);
		/* Initialize the item before adding it to a bucket. */
		memory_barrier();

		mark_t pred_mark = get_mark(*wnd->ppred);
		/* If the predecessor is a join node it may be marked deleted.*/
		mark_t exp_pred_mark = (N_JOIN & pred_mark) ? pred_mark : N_NORMAL;

		ret = cas_link(wnd->ppred, wnd->cur, exp_pred_mark, item, exp_pred_mark);
		return ret == make_link(wnd->cur, exp_pred_mark);
	}
}

/** Returns true if the chain starting at cur has an item equal to \a item.
 *
 * @param h    CHT to operate on.
 * @param item Item whose duplicates the function looks for.
 * @param hash Hash of \a item.
 * @param[in] cur  The first node with a hash greater to or equal to item's hash.
 * @param[out] dup_item The first duplicate item encountered.
 * @return True if a non-deleted item equal to \a item exists in the table.
 */
static inline bool has_duplicate(cht_t *h, const cht_link_t *item, size_t hash,
	cht_link_t *cur, cht_link_t **dup_item)
{
	assert(cur);
	assert(cur == &sentinel || hash <= node_hash(h, cur)
		|| node_hash(h, cur) == h->invalid_hash);

	/* hash < node_hash(h, cur) */
	if (hash != node_hash(h, cur) && h->invalid_hash != node_hash(h, cur))
		return false;

	/*
	 * Load the most recent node marks. Otherwise we might pronounce a
	 * logically deleted node for a duplicate of the item just because
	 * the deleted node's DEL mark had not yet propagated to this cpu.
	 */
	read_barrier();

	*dup_item = find_duplicate(h, item, hash, cur);
	return NULL != *dup_item;
}

/** Returns an item that is equal to \a item starting in a chain at \a start. */
static cht_link_t *find_duplicate(cht_t *h, const cht_link_t *item, size_t hash,
	cht_link_t *start)
{
	assert(hash <= node_hash(h, start) || h->invalid_hash == node_hash(h, start));

	cht_link_t *cur = start;

try_again:
	assert(cur);

	while (node_hash(h, cur) == hash) {
		assert(cur != &sentinel);

		bool deleted = (N_DELETED & get_mark(cur->link));

		/* Skip logically deleted nodes. */
		if (!deleted && h->op->equal(item, cur))
			return cur;

		cur = get_next(cur->link);
		assert(cur);
	}

	/* Skip logically deleted nodes with rcu_call() in progress. */
	if (h->invalid_hash == node_hash(h, cur)) {
		cur = get_next(cur->link);
		goto try_again;
	}

	return NULL;
}

/** Removes all items matching the search key. Returns the number of items removed.*/
size_t cht_remove_key(cht_t *h, void *key)
{
	assert(h);

	size_t hash = calc_key_hash(h, key);
	size_t removed = 0;

	while (remove_pred(h, hash, h->op->key_equal, key))
		++removed;

	return removed;
}

/** Removes a specific item from the table.
 *
 * The called must hold rcu read lock.
 *
 * @param item Item presumably present in the table and to be removed.
 * @return True if the item was removed successfully; or false if it had
 *     already been deleted.
 */
bool cht_remove_item(cht_t *h, cht_link_t *item)
{
	assert(h);
	assert(item);
	/* Otherwise a concurrent cht_remove_key might free the item. */
	assert(rcu_read_locked());

	/*
	 * Even though we know the node we want to delete we must unlink it
	 * from the correct bucket and from a clean/normal predecessor. Therefore,
	 * we search for it again from the beginning of the correct bucket.
	 */
	size_t hash = calc_node_hash(h, item);
	return remove_pred(h, hash, same_node_pred, item);
}

/** Removes an item equal to pred_arg according to the predicate pred. */
static bool remove_pred(cht_t *h, size_t hash, equal_pred_t pred, void *pred_arg)
{
	rcu_read_lock();

	bool resizing = false;
	bool deleted = false;
	bool deleted_but_gc = false;

	cht_buckets_t *b = rcu_access(h->b);
	size_t idx = calc_bucket_idx(hash, b->order);
	marked_ptr_t *phead = &b->head[idx];

	do {
		walk_mode_t walk_mode = WM_NORMAL;
		bool join_finishing = false;

		resizing = resizing || (N_NORMAL != get_mark(*phead));

		/* The table is resizing. Get the correct bucket head. */
		if (resizing) {
			upd_resizing_head(h, hash, &phead, &join_finishing, &walk_mode);
		}

		wnd_t wnd = {
			.ppred = phead,
			.cur = get_next(*phead),
			.last = NULL
		};

		if (!find_wnd_and_gc_pred(
			h, hash, walk_mode, pred, pred_arg, &wnd, &resizing)) {
			/* Could not GC a node; or detected an unexpected resize. */
			continue;
		}

		/*
		 * The item lookup is affected by a bucket join but effects of
		 * the bucket join have not been seen while searching for the item.
		 */
		if (join_finishing && !join_completed(h, &wnd)) {
			/*
			 * Bucket was appended at the end of another but the next
			 * ptr linking them together was not visible on this cpu.
			 * join_completed() makes this appended bucket visible.
			 */
			continue;
		}

		/* Already deleted, but delete_at() requested one GC pass. */
		if (deleted_but_gc)
			break;

		bool found = (wnd.cur != &sentinel && pred(pred_arg, wnd.cur));

		if (!found) {
			rcu_read_unlock();
			return false;
		}

		deleted = delete_at(h, &wnd, walk_mode, &deleted_but_gc, &resizing);
	} while (!deleted || deleted_but_gc);

	rcu_read_unlock();
	return true;
}

/** Unlinks wnd.cur from wnd.ppred and schedules a deferred free for the item.
 *
 * Ignores nodes marked N_JOIN if walk mode is WM_LEAVE_JOIN.
 *
 * @param h   CHT to operate on.
 * @param wnd Points to the item to delete and its N_NORMAL predecessor.
 * @param walk_mode Bucket chaing walk mode.
 * @param deleted_but_gc Set to true if the item had been logically deleted,
 *         but a garbage collecting walk of the bucket is in order for
 *         it to be fully unlinked.
 * @param resizing Set to true if the table is undergoing an unexpected
 *         resize (ie walk_mode == WM_NORMAL).
 * @return False if the wnd.ppred changed in the meantime and the whole
 *         delete operation must be retried.
 */
static inline bool delete_at(cht_t *h, wnd_t *wnd, walk_mode_t walk_mode,
	bool *deleted_but_gc, bool *resizing)
{
	assert(wnd->cur && wnd->cur != &sentinel);

	*deleted_but_gc = false;

	if (!mark_deleted(wnd->cur, walk_mode, resizing)) {
		/* Already deleted, or unexpectedly marked as JOIN/JOIN_FOLLOWS. */
		return false;
	}

	/* Marked deleted. Unlink from the bucket. */

	/* Never unlink join nodes. */
	if (walk_mode == WM_LEAVE_JOIN && (N_JOIN & get_mark(wnd->cur->link)))
		return true;

	cas_order_barrier();

	if (unlink_from_pred(wnd, walk_mode, resizing)) {
		free_later(h, wnd->cur);
	} else {
		*deleted_but_gc = true;
	}

	return true;
}

/** Marks cur logically deleted. Returns false to request a retry. */
static inline bool mark_deleted(cht_link_t *cur, walk_mode_t walk_mode,
	bool *resizing)
{
	assert(cur && cur != &sentinel);

	/*
	 * Btw, we could loop here if the cas fails but let's not complicate
	 * things and let's retry from the head of the bucket.
	 */

	cht_link_t *next = get_next(cur->link);

	if (walk_mode == WM_NORMAL) {
		/* Only mark clean/normal nodes - JF/JN is used only during resize. */
		marked_ptr_t ret = cas_link(&cur->link, next, N_NORMAL, next, N_DELETED);

		if (ret != make_link(next, N_NORMAL)) {
			*resizing = (N_JOIN | N_JOIN_FOLLOWS) & get_mark(ret);
			return false;
		}
	} else {
		static_assert(N_JOIN == N_JOIN_FOLLOWS, "");

		/* Keep the N_JOIN/N_JOIN_FOLLOWS mark but strip N_DELETED. */
		mark_t cur_mark = get_mark(cur->link) & N_JOIN_FOLLOWS;

		marked_ptr_t ret =
			cas_link(&cur->link, next, cur_mark, next, cur_mark | N_DELETED);

		if (ret != make_link(next, cur_mark))
			return false;
	}

	return true;
}

/** Unlinks wnd.cur from wnd.ppred. Returns false if it should be retried. */
static inline bool unlink_from_pred(wnd_t *wnd, walk_mode_t walk_mode,
	bool *resizing)
{
	assert(wnd->cur != &sentinel);
	assert(wnd->cur && (N_DELETED & get_mark(wnd->cur->link)));

	cht_link_t *next = get_next(wnd->cur->link);

	if (walk_mode == WM_LEAVE_JOIN) {
		/* Never try to unlink join nodes. */
		assert(!(N_JOIN & get_mark(wnd->cur->link)));

		mark_t pred_mark = get_mark(*wnd->ppred);
		/* Succeed only if the predecessor is clean/normal or a join node. */
		mark_t exp_pred_mark = (N_JOIN & pred_mark) ? pred_mark : N_NORMAL;

		marked_ptr_t pred_link = make_link(wnd->cur, exp_pred_mark);
		marked_ptr_t next_link = make_link(next, exp_pred_mark);

		if (pred_link != _cas_link(wnd->ppred, pred_link, next_link))
			return false;
	} else {
		assert(walk_mode == WM_MOVE_JOIN_FOLLOWS || walk_mode == WM_NORMAL);
		/* Move the JF mark if set. Clear DEL mark. */
		mark_t cur_mark = N_JOIN_FOLLOWS & get_mark(wnd->cur->link);

		/* The predecessor must be clean/normal. */
		marked_ptr_t pred_link = make_link(wnd->cur, N_NORMAL);
		/* Link to cur's successor keeping/copying cur's JF mark. */
		marked_ptr_t next_link = make_link(next, cur_mark);

		marked_ptr_t ret = _cas_link(wnd->ppred, pred_link, next_link);

		if (pred_link != ret) {
			/* If we're not resizing the table there are no JF/JN nodes. */
			*resizing = (walk_mode == WM_NORMAL)
				&& (N_JOIN_FOLLOWS & get_mark(ret));
			return false;
		}
	}

	return true;
}

/** Finds the first non-deleted item equal to \a pred_arg according to \a pred.
 *
 * The function returns the candidate item in \a wnd. Logically deleted
 * nodes are garbage collected so the predecessor will most likely not
 * be marked as deleted.
 *
 * Unlike find_wnd_and_gc(), this function never returns a node that
 * is known to have already been marked N_DELETED.
 *
 * Any logically deleted nodes (ie those marked N_DELETED) are garbage
 * collected, ie free in the background via rcu_call (except for join-nodes
 * if walk_mode == WM_LEAVE_JOIN).
 *
 * @param h         CHT to operate on.
 * @param hash      Hash the search for.
 * @param walk_mode Bucket chain walk mode.
 * @param pred      Predicate used to find an item equal to pred_arg.
 * @param pred_arg  Argument to pass to the equality predicate \a pred.
 * @param[in,out] wnd The search starts with wnd.cur. If the desired
 *                  item is found wnd.cur will point to it.
 * @param resizing  Set to true if the table is resizing but it was not
 *                  expected (ie walk_mode == WM_NORMAL).
 * @return False if the operation has to be retried. True otherwise
 *        (even if an equal item had not been found).
 */
static bool find_wnd_and_gc_pred(cht_t *h, size_t hash, walk_mode_t walk_mode,
	equal_pred_t pred, void *pred_arg, wnd_t *wnd, bool *resizing)
{
	assert(wnd->cur);

	if (wnd->cur == &sentinel)
		return true;

	/*
	 * A read barrier is not needed here to bring up the most recent
	 * node marks (esp the N_DELETED). At worst we'll try to delete
	 * an already deleted node; fail in delete_at(); and retry.
	 */

	size_t cur_hash;

try_again:
	cur_hash = node_hash(h, wnd->cur);

	while (cur_hash <= hash) {
		assert(wnd->cur && wnd->cur != &sentinel);

		/* GC any deleted nodes on the way. */
		if (N_DELETED & get_mark(wnd->cur->link)) {
			if (!gc_deleted_node(h, walk_mode, wnd, resizing)) {
				/* Retry from the head of a bucket. */
				return false;
			}
		} else {
			/* Is this the node we were looking for? */
			if (cur_hash == hash && pred(pred_arg, wnd->cur))
				return true;

			next_wnd(wnd);
		}

		cur_hash = node_hash(h, wnd->cur);
	}

	if (cur_hash == h->invalid_hash) {
		next_wnd(wnd);
		assert(wnd->cur);
		goto try_again;
	}

	/* The searched for node is not in the current bucket. */
	return true;
}

/** Find the first item (deleted or not) with a hash greater or equal to \a hash.
 *
 * The function returns the first item with a hash that is greater or
 * equal to \a hash in \a wnd. Moreover it garbage collects logically
 * deleted node that have not yet been unlinked and freed. Therefore,
 * the returned node's predecessor will most likely be N_NORMAL.
 *
 * Unlike find_wnd_and_gc_pred(), this function may return a node
 * that is known to had been marked N_DELETED.
 *
 * @param h         CHT to operate on.
 * @param hash      Hash of the item to find.
 * @param walk_mode Bucket chain walk mode.
 * @param[in,out] wnd wnd.cur denotes the first node of the chain. If the
 *                  the operation is successful, \a wnd points to the desired
 *                  item.
 * @param resizing  Set to true if a table resize was detected but walk_mode
 *                  suggested the table was not undergoing a resize.
 * @return False indicates the operation must be retried. True otherwise
 *       (even if an item with exactly the same has was not found).
 */
static bool find_wnd_and_gc(cht_t *h, size_t hash, walk_mode_t walk_mode,
	wnd_t *wnd, bool *resizing)
{
try_again:
	assert(wnd->cur);

	while (node_hash(h, wnd->cur) < hash) {
		/* GC any deleted nodes along the way to our desired node. */
		if (N_DELETED & get_mark(wnd->cur->link)) {
			if (!gc_deleted_node(h, walk_mode, wnd, resizing)) {
				/* Failed to remove the garbage node. Retry. */
				return false;
			}
		} else {
			next_wnd(wnd);
		}

		assert(wnd->cur);
	}

	if (node_hash(h, wnd->cur) == h->invalid_hash) {
		next_wnd(wnd);
		goto try_again;
	}

	/* wnd->cur may be NULL or even marked N_DELETED. */
	return true;
}

/** Garbage collects the N_DELETED node at \a wnd skipping join nodes. */
static bool gc_deleted_node(cht_t *h, walk_mode_t walk_mode, wnd_t *wnd,
	bool *resizing)
{
	assert(N_DELETED & get_mark(wnd->cur->link));

	/* Skip deleted JOIN nodes. */
	if (walk_mode == WM_LEAVE_JOIN && (N_JOIN & get_mark(wnd->cur->link))) {
		next_wnd(wnd);
	} else {
		/* Ordinary deleted node or a deleted JOIN_FOLLOWS. */
		assert(walk_mode != WM_LEAVE_JOIN
			|| !((N_JOIN | N_JOIN_FOLLOWS) & get_mark(wnd->cur->link)));

		/* Unlink an ordinary deleted node, move JOIN_FOLLOWS mark. */
		if (!unlink_from_pred(wnd, walk_mode, resizing)) {
			/* Retry. The predecessor was deleted, invalid, const, join_follows. */
			return false;
		}

		free_later(h, wnd->cur);

		/* Leave ppred as is. */
		wnd->last = wnd->cur;
		wnd->cur = get_next(wnd->cur->link);
	}

	return true;
}

/** Returns true if a bucket join had already completed.
 *
 * May only be called if upd_resizing_head() indicates a bucket join
 * may be in progress.
 *
 * If it returns false, the search must be retried in order to guarantee
 * all item that should have been encountered have been seen.
 */
static bool join_completed(cht_t *h, const wnd_t *wnd)
{
	/*
	 * The table is shrinking and the searched for item is in a bucket
	 * appended to another. Check that the link joining these two buckets
	 * is visible and if not, make it visible to this cpu.
	 */

	/*
	 * Resizer ensures h->b->order stays the same for the duration of this
	 * func. We got here because there was an alternative head to search.
	 * The resizer waits for all preexisting readers to finish after
	 * it
	 */
	assert(h->b->order > h->new_b->order);
	assert(wnd->cur);

	/* Either we did not need the joining link or we have already followed it.*/
	if (wnd->cur != &sentinel)
		return true;

	/* We have reached the end of a bucket. */

	if (wnd->last != &sentinel) {
		size_t last_seen_hash = node_hash(h, wnd->last);

		if (last_seen_hash == h->invalid_hash) {
			last_seen_hash = calc_node_hash(h, wnd->last);
		}

		size_t last_old_idx = calc_bucket_idx(last_seen_hash, h->b->order);
		size_t move_src_idx = grow_idx(shrink_idx(last_old_idx));

		/*
		 * Last node seen was in the joining bucket - if the searched
		 * for node is there we will find it.
		 */
		if (move_src_idx != last_old_idx)
			return true;
	}

	/*
	 * Reached the end of the bucket but no nodes from the joining bucket
	 * were seen. There should have at least been a JOIN node so we have
	 * definitely not seen (and followed) the joining link. Make the link
	 * visible and retry.
	 */
	read_barrier();
	return false;
}

/** When resizing returns the bucket head to start the search with in \a phead.
 *
 * If a resize had been detected (eg cht_t.b.head[idx] is marked immutable).
 * upd_resizing_head() moves the bucket for \a hash from the old head
 * to the new head. Moreover, it splits or joins buckets as necessary.
 *
 * @param h     CHT to operate on.
 * @param hash  Hash of an item whose chain we would like to traverse.
 * @param[out] phead Head of the bucket to search for \a hash.
 * @param[out] join_finishing Set to true if a bucket join might be
 *              in progress and the bucket may have to traversed again
 *              as indicated by join_completed().
 * @param[out] walk_mode Specifies how to interpret node marks.
 */
static void upd_resizing_head(cht_t *h, size_t hash, marked_ptr_t **phead,
	bool *join_finishing,  walk_mode_t *walk_mode)
{
	cht_buckets_t *b = rcu_access(h->b);
	size_t old_idx = calc_bucket_idx(hash, b->order);
	size_t new_idx = calc_bucket_idx(hash, h->new_b->order);

	marked_ptr_t *pold_head = &b->head[old_idx];
	marked_ptr_t *pnew_head = &h->new_b->head[new_idx];

	/* In any case, use the bucket in the new table. */
	*phead = pnew_head;

	/* Growing the table. */
	if (b->order < h->new_b->order) {
		size_t move_dest_idx = grow_idx(old_idx);
		marked_ptr_t *pmoved_head = &h->new_b->head[move_dest_idx];

		/* Complete moving the bucket from the old to the new table. */
		help_head_move(pold_head, pmoved_head);

		/* The hash belongs to the moved bucket. */
		if (move_dest_idx == new_idx) {
			assert(pmoved_head == pnew_head);
			/*
			 * move_head() makes the new head of the moved bucket visible.
			 * The new head may be marked with a JOIN_FOLLOWS
			 */
			assert(!(N_CONST & get_mark(*pmoved_head)));
			*walk_mode = WM_MOVE_JOIN_FOLLOWS;
		} else {
			assert(pmoved_head != pnew_head);
			/*
			 * The hash belongs to the bucket that is the result of splitting
			 * the old/moved bucket, ie the bucket that contains the second
			 * half of the split/old/moved bucket.
			 */

			/* The moved bucket has not yet been split. */
			if (N_NORMAL != get_mark(*pnew_head)) {
				size_t split_hash = calc_split_hash(new_idx, h->new_b->order);
				split_bucket(h, pmoved_head, pnew_head, split_hash);
				/*
				 * split_bucket() makes the new head visible. No
				 * JOIN_FOLLOWS in this part of split bucket.
				 */
				assert(N_NORMAL == get_mark(*pnew_head));
			}

			*walk_mode = WM_LEAVE_JOIN;
		}
	} else if (h->new_b->order < b->order ) {
		/* Shrinking the table. */

		size_t move_src_idx = grow_idx(new_idx);

		/*
		 * Complete moving the bucket from the old to the new table.
		 * Makes a valid pnew_head visible if already moved.
		 */
		help_head_move(&b->head[move_src_idx], pnew_head);

		/* Hash belongs to the bucket to be joined with the moved bucket. */
		if (move_src_idx != old_idx) {
			/* Bucket join not yet completed. */
			if (N_INVALID != get_mark(*pold_head)) {
				size_t split_hash = calc_split_hash(old_idx, b->order);
				join_buckets(h, pold_head, pnew_head, split_hash);
			}

			/*
			 * The resizer sets pold_head to &sentinel when all cpus are
			 * guaranteed to see the bucket join.
			 */
			*join_finishing = (&sentinel != get_next(*pold_head));
		}

		/* move_head() or join_buckets() makes it so or makes the mark visible.*/
		assert(N_INVALID == get_mark(*pold_head));
		/* move_head() makes it visible. No JOIN_FOLLOWS used when shrinking. */
		assert(N_NORMAL == get_mark(*pnew_head));

		*walk_mode = WM_LEAVE_JOIN;
	} else {
		/*
		 * Final stage of resize. The resizer is waiting for all
		 * readers to notice that the old table had been replaced.
		 */
		assert(b == h->new_b);
		*walk_mode = WM_NORMAL;
	}
}


#if 0
static void move_head(marked_ptr_t *psrc_head, marked_ptr_t *pdest_head)
{
	start_head_move(psrc_head);
	cas_order_barrier();
	complete_head_move(psrc_head, pdest_head);
}
#endif

/** Moves an immutable head \a psrc_head of cht_t.b to \a pdest_head of cht_t.new_b.
 *
 * The function guarantees the move will be visible on this cpu once
 * it completes. In particular, *pdest_head will not be N_INVALID.
 *
 * Unlike complete_head_move(), help_head_move() checks if the head had already
 * been moved and tries to avoid moving the bucket heads if possible.
 */
static inline void help_head_move(marked_ptr_t *psrc_head,
	marked_ptr_t *pdest_head)
{
	/* Head move has to in progress already when calling this func. */
	assert(N_CONST & get_mark(*psrc_head));

	/* Head already moved. */
	if (N_INVALID == get_mark(*psrc_head)) {
		/* Effects of the head move have not yet propagated to this cpu. */
		if (N_INVALID == get_mark(*pdest_head)) {
			/* Make the move visible on this cpu. */
			read_barrier();
		}
	} else {
		complete_head_move(psrc_head, pdest_head);
	}

	assert(!(N_CONST & get_mark(*pdest_head)));
}

/** Initiates the move of the old head \a psrc_head.
 *
 * The move may be completed with help_head_move().
 */
static void start_head_move(marked_ptr_t *psrc_head)
{
	/* Mark src head immutable. */
	mark_const(psrc_head);
}

/** Marks the head immutable. */
static void mark_const(marked_ptr_t *psrc_head)
{
	marked_ptr_t ret, src_link;

	/* Mark src head immutable. */
	do {
		cht_link_t *next = get_next(*psrc_head);
		src_link = make_link(next, N_NORMAL);

		/* Mark the normal/clean src link immutable/const. */
		ret = cas_link(psrc_head, next, N_NORMAL, next, N_CONST);
	} while(ret != src_link && !(N_CONST & get_mark(ret)));
}

/** Completes moving head psrc_head to pdest_head (started by start_head_move()).*/
static void complete_head_move(marked_ptr_t *psrc_head, marked_ptr_t *pdest_head)
{
	assert(N_JOIN_FOLLOWS != get_mark(*psrc_head));
	assert(N_CONST & get_mark(*psrc_head));

	cht_link_t *next = get_next(*psrc_head);

	DBG(marked_ptr_t ret = )
		cas_link(pdest_head, &sentinel, N_INVALID, next, N_NORMAL);
	assert(ret == make_link(&sentinel, N_INVALID) || (N_NORMAL == get_mark(ret)));
	cas_order_barrier();

	DBG(ret = )
		cas_link(psrc_head, next, N_CONST, next, N_INVALID);
	assert(ret == make_link(next, N_CONST) || (N_INVALID == get_mark(ret)));
	cas_order_barrier();
}

/** Splits the bucket at psrc_head and links to the remainder from pdest_head.
 *
 * Items with hashes greater or equal to \a split_hash are moved to bucket
 * with head at \a pdest_head.
 *
 * @param h           CHT to operate on.
 * @param psrc_head   Head of the bucket to split (in cht_t.new_b).
 * @param pdest_head  Head of the bucket that points to the second part
 *                    of the split bucket in psrc_head. (in cht_t.new_b)
 * @param split_hash  Hash of the first possible item in the remainder of
 *                    psrc_head, ie the smallest hash pdest_head is allowed
 *                    to point to..
 */
static void split_bucket(cht_t *h, marked_ptr_t *psrc_head,
	marked_ptr_t *pdest_head, size_t split_hash)
{
	/* Already split. */
	if (N_NORMAL == get_mark(*pdest_head))
		return;

	/*
	 * L == Last node of the first part of the split bucket. That part
	 *      remains in the original/src bucket.
	 * F == First node of the second part of the split bucket. That part
	 *      will be referenced from the dest bucket head.
	 *
	 * We want to first mark a clean L as JF so that updaters unaware of
	 * the split (or table resize):
	 * - do not insert a new node between L and F
	 * - do not unlink L (that is why it has to be clean/normal)
	 * - do not unlink F
	 *
	 * Then we can safely mark F as JN even if it has been marked deleted.
	 * Once F is marked as JN updaters aware of table resize will not
	 * attempt to unlink it (JN will have two predecessors - we cannot
	 * safely unlink from both at the same time). Updaters unaware of
	 * ongoing resize can reach F only via L and that node is already
	 * marked JF, so they won't unlink F.
	 *
	 * Last, link the new/dest head to F.
	 *
	 *
	 * 0)                           ,-- split_hash, first hash of the dest bucket
	 *                              v
	 *  [src_head | N] -> .. -> [L] -> [F]
	 *  [dest_head | Inv]
	 *
	 * 1)                             ,-- split_hash
	 *                                v
	 *  [src_head | N] -> .. -> [JF] -> [F]
	 *  [dest_head | Inv]
	 *
	 * 2)                             ,-- split_hash
	 *                                v
	 *  [src_head | N] -> .. -> [JF] -> [JN]
	 *  [dest_head | Inv]
	 *
	 * 3)                             ,-- split_hash
	 *                                v
	 *  [src_head | N] -> .. -> [JF] -> [JN]
	 *                                   ^
	 *  [dest_head | N] -----------------'
	 */
	wnd_t wnd;

	rcu_read_lock();

	/* Mark the last node of the first part of the split bucket as JF. */
	mark_join_follows(h, psrc_head, split_hash, &wnd);
	cas_order_barrier();

	/* There are nodes in the dest bucket, ie the second part of the split. */
	if (wnd.cur != &sentinel) {
		/*
		 * Mark the first node of the dest bucket as a join node so
		 * updaters do not attempt to unlink it if it is deleted.
		 */
		mark_join_node(wnd.cur);
		cas_order_barrier();
	} else {
		/*
		 * Second part of the split bucket is empty. There are no nodes
		 * to mark as JOIN nodes and there never will be.
		 */
	}

	/* Link the dest head to the second part of the split. */
	DBG(marked_ptr_t ret = )
		cas_link(pdest_head, &sentinel, N_INVALID, wnd.cur, N_NORMAL);
	assert(ret == make_link(&sentinel, N_INVALID) || (N_NORMAL == get_mark(ret)));
	cas_order_barrier();

	rcu_read_unlock();
}

/** Finds and marks the last node of psrc_head w/ hash less than split_hash.
 *
 * Finds a node in psrc_head with the greatest hash that is strictly less
 * than split_hash and marks it with N_JOIN_FOLLOWS.
 *
 * Returns a window pointing to that node.
 *
 * Any logically deleted nodes along the way are
 * garbage collected; therefore, the predecessor node (if any) will most
 * likely not be marked N_DELETED.
 *
 * @param h          CHT to operate on.
 * @param psrc_head  Bucket head.
 * @param split_hash The smallest hash a join node (ie the node following
 *                   the desired join-follows node) may have.
 * @param[out] wnd   Points to the node marked with N_JOIN_FOLLOWS.
 */
static void mark_join_follows(cht_t *h, marked_ptr_t *psrc_head,
	size_t split_hash, wnd_t *wnd)
{
	/* See comment in split_bucket(). */

	bool done = false;

	do {
		bool resizing = false;
		wnd->ppred = psrc_head;
		wnd->cur = get_next(*psrc_head);

		/*
		 * Find the split window, ie the last node of the first part of
		 * the split bucket and the its successor - the first node of
		 * the second part of the split bucket. Retry if GC failed.
		 */
		if (!find_wnd_and_gc(h, split_hash, WM_MOVE_JOIN_FOLLOWS, wnd, &resizing))
			continue;

		/* Must not report that the table is resizing if WM_MOVE_JOIN_FOLLOWS.*/
		assert(!resizing);
		/*
		 * Mark the last node of the first half of the split bucket
		 * that a join node follows. It must be clean/normal.
		 */
		marked_ptr_t ret
			= cas_link(wnd->ppred, wnd->cur, N_NORMAL, wnd->cur, N_JOIN_FOLLOWS);

		/*
		 * Successfully marked as a JF node or already marked that way (even
		 * if also marked deleted - unlinking the node will move the JF mark).
		 */
		done = (ret == make_link(wnd->cur, N_NORMAL))
			|| (N_JOIN_FOLLOWS & get_mark(ret));
	} while (!done);
}

/** Marks join_node with N_JOIN. */
static void mark_join_node(cht_link_t *join_node)
{
	/* See comment in split_bucket(). */

	bool done;
	do {
		cht_link_t *next = get_next(join_node->link);
		mark_t mark = get_mark(join_node->link);

		/*
		 * May already be marked as deleted, but it won't be unlinked
		 * because its predecessor is marked with JOIN_FOLLOWS or CONST.
		 */
		marked_ptr_t ret
			= cas_link(&join_node->link, next, mark, next, mark | N_JOIN);

		/* Successfully marked or already marked as a join node. */
		done = (ret == make_link(next, mark))
			|| (N_JOIN & get_mark(ret));
	} while(!done);
}

/** Appends the bucket at psrc_head to the bucket at pdest_head.
 *
 * @param h          CHT to operate on.
 * @param psrc_head  Bucket to merge with pdest_head.
 * @param pdest_head Bucket to be joined by psrc_head.
 * @param split_hash The smallest hash psrc_head may contain.
 */
static void join_buckets(cht_t *h, marked_ptr_t *psrc_head,
	marked_ptr_t *pdest_head, size_t split_hash)
{
	/* Buckets already joined. */
	if (N_INVALID == get_mark(*psrc_head))
		return;
	/*
	 * F == First node of psrc_head, ie the bucket we want to append
	 *      to (ie join with) the bucket starting at pdest_head.
	 * L == Last node of pdest_head, ie the bucket that psrc_head will
	 *      be appended to.
	 *
	 * (1) We first mark psrc_head immutable to signal that a join is
	 * in progress and so that updaters unaware of the join (or table
	 * resize):
	 * - do not insert new nodes between the head psrc_head and F
	 * - do not unlink F (it may already be marked deleted)
	 *
	 * (2) Next, F is marked as a join node. Updaters aware of table resize
	 * will not attempt to unlink it. We cannot safely/atomically unlink
	 * the join node because it will be pointed to from two different
	 * buckets. Updaters unaware of resize will fail to unlink the join
	 * node due to the head being marked immutable.
	 *
	 * (3) Then the tail of the bucket at pdest_head is linked to the join
	 * node. From now on, nodes in both buckets can be found via pdest_head.
	 *
	 * (4) Last, mark immutable psrc_head as invalid. It signals updaters
	 * that the join is complete and they can insert new nodes (originally
	 * destined for psrc_head) into pdest_head.
	 *
	 * Note that pdest_head keeps pointing at the join node. This allows
	 * lookups and updaters to determine if they should see a link between
	 * the tail L and F when searching for nodes originally in psrc_head
	 * via pdest_head. If they reach the tail of pdest_head without
	 * encountering any nodes of psrc_head, either there were no nodes
	 * in psrc_head to begin with or the link between L and F did not
	 * yet propagate to their cpus. If psrc_head was empty, it remains
	 * NULL. Otherwise psrc_head points to a join node (it will not be
	 * unlinked until table resize completes) and updaters/lookups
	 * should issue a read_barrier() to make the link [L]->[JN] visible.
	 *
	 * 0)                           ,-- split_hash, first hash of the src bucket
	 *                              v
	 *  [dest_head | N]-> .. -> [L]
	 *  [src_head | N]--> [F] -> ..
	 *  ^
	 *  ` split_hash, first hash of the src bucket
	 *
	 * 1)                            ,-- split_hash
	 *                               v
	 *  [dest_head | N]-> .. -> [L]
	 *  [src_head | C]--> [F] -> ..
	 *
	 * 2)                            ,-- split_hash
	 *                               v
	 *  [dest_head | N]-> .. -> [L]
	 *  [src_head | C]--> [JN] -> ..
	 *
	 * 3)                            ,-- split_hash
	 *                               v
	 *  [dest_head | N]-> .. -> [L] --+
	 *                                v
	 *  [src_head | C]-------------> [JN] -> ..
	 *
	 * 4)                            ,-- split_hash
	 *                               v
	 *  [dest_head | N]-> .. -> [L] --+
	 *                                v
	 *  [src_head | Inv]-----------> [JN] -> ..
	 */

	rcu_read_lock();

	/* Mark src_head immutable - signals updaters that bucket join started. */
	mark_const(psrc_head);
	cas_order_barrier();

	cht_link_t *join_node = get_next(*psrc_head);

	if (join_node != &sentinel) {
		mark_join_node(join_node);
		cas_order_barrier();

		link_to_join_node(h, pdest_head, join_node, split_hash);
		cas_order_barrier();
	}

	DBG(marked_ptr_t ret = )
		cas_link(psrc_head, join_node, N_CONST, join_node, N_INVALID);
	assert(ret == make_link(join_node, N_CONST) || (N_INVALID == get_mark(ret)));
	cas_order_barrier();

	rcu_read_unlock();
}

/** Links the tail of pdest_head to join_node.
 *
 * @param h          CHT to operate on.
 * @param pdest_head Head of the bucket whose tail is to be linked to join_node.
 * @param join_node  A node marked N_JOIN with a hash greater or equal to
 *                   split_hash.
 * @param split_hash The least hash that is greater than the hash of any items
 *                   (originally) in pdest_head.
 */
static void link_to_join_node(cht_t *h, marked_ptr_t *pdest_head,
	cht_link_t *join_node, size_t split_hash)
{
	bool done = false;

	do {
		wnd_t wnd = {
			.ppred = pdest_head,
			.cur = get_next(*pdest_head)
		};

		bool resizing = false;

		if (!find_wnd_and_gc(h, split_hash, WM_LEAVE_JOIN, &wnd, &resizing))
			continue;

		assert(!resizing);

		if (wnd.cur != &sentinel) {
			/* Must be from the new appended bucket. */
			assert(split_hash <= node_hash(h, wnd.cur)
				|| h->invalid_hash == node_hash(h, wnd.cur));
			return;
		}

		/* Reached the tail of pdest_head - link it to the join node. */
		marked_ptr_t ret =
			cas_link(wnd.ppred, &sentinel, N_NORMAL, join_node, N_NORMAL);

		done = (ret == make_link(&sentinel, N_NORMAL));
	} while (!done);
}

/** Instructs RCU to free the item once all preexisting references are dropped.
 *
 * The item is freed via op->remove_callback().
 */
static void free_later(cht_t *h, cht_link_t *item)
{
	assert(item != &sentinel);

	/*
	 * remove_callback only works as rcu_func_t because rcu_link is the first
	 * field in cht_link_t.
	 */
	rcu_call(&item->rcu_link, (rcu_func_t)h->op->remove_callback);

	item_removed(h);
}

/** Notes that an item had been unlinked from the table and shrinks it if needed.
 *
 * If the number of items in the table drops below 1/4 of the maximum
 * allowed load the table is shrunk in the background.
 */
static inline void item_removed(cht_t *h)
{
	size_t items = (size_t) atomic_predec(&h->item_cnt);
	size_t bucket_cnt = (1 << h->b->order);

	bool need_shrink = (items == h->max_load * bucket_cnt / 4);
	bool missed_shrink = (items == h->max_load * bucket_cnt / 8);

	if ((need_shrink || missed_shrink) && h->b->order > h->min_order) {
		atomic_count_t resize_reqs = atomic_preinc(&h->resize_reqs);
		/* The first resize request. Start the resizer. */
		if (1 == resize_reqs) {
			workq_global_enqueue_noblock(&h->resize_work, resize_table);
		}
	}
}

/** Notes an item had been inserted and grows the table if needed.
 *
 * The table is resized in the background.
 */
static inline void item_inserted(cht_t *h)
{
	size_t items = (size_t) atomic_preinc(&h->item_cnt);
	size_t bucket_cnt = (1 << h->b->order);

	bool need_grow = (items == h->max_load * bucket_cnt);
	bool missed_grow = (items == 2 * h->max_load * bucket_cnt);

	if ((need_grow || missed_grow) && h->b->order < CHT_MAX_ORDER) {
		atomic_count_t resize_reqs = atomic_preinc(&h->resize_reqs);
		/* The first resize request. Start the resizer. */
		if (1 == resize_reqs) {
			workq_global_enqueue_noblock(&h->resize_work, resize_table);
		}
	}
}

/** Resize request handler. Invoked on the system work queue. */
static void resize_table(work_t *arg)
{
	cht_t *h = member_to_inst(arg, cht_t, resize_work);

#ifdef CONFIG_DEBUG
	assert(h->b);
	/* Make resize_reqs visible. */
	read_barrier();
	assert(0 < atomic_get(&h->resize_reqs));
#endif

	bool done = false;

	do {
		/* Load the most recent h->item_cnt. */
		read_barrier();
		size_t cur_items = (size_t) atomic_get(&h->item_cnt);
		size_t bucket_cnt = (1 << h->b->order);
		size_t max_items = h->max_load * bucket_cnt;

		if (cur_items >= max_items && h->b->order < CHT_MAX_ORDER) {
			grow_table(h);
		} else if (cur_items <= max_items / 4 && h->b->order > h->min_order) {
			shrink_table(h);
		} else {
			/* Table is just the right size. */
			atomic_count_t reqs = atomic_predec(&h->resize_reqs);
			done = (reqs == 0);
		}
	} while (!done);
}

/** Increases the number of buckets two-fold. Blocks until done. */
static void grow_table(cht_t *h)
{
	if (h->b->order >= CHT_MAX_ORDER)
		return;

	h->new_b = alloc_buckets(h->b->order + 1, true, false);

	/* Failed to alloc a new table - try next time the resizer is run. */
	if (!h->new_b)
		return;

	/* Wait for all readers and updaters to see the initialized new table. */
	rcu_synchronize();
	size_t old_bucket_cnt = (1 << h->b->order);

	/*
	 * Give updaters a chance to help out with the resize. Do the minimum
	 * work needed to announce a resize is in progress, ie start moving heads.
	 */
	for (size_t idx = 0; idx < old_bucket_cnt; ++idx) {
		start_head_move(&h->b->head[idx]);
	}

	/* Order start_head_move() wrt complete_head_move(). */
	cas_order_barrier();

	/* Complete moving heads and split any buckets not yet split by updaters. */
	for (size_t old_idx = 0; old_idx < old_bucket_cnt; ++old_idx) {
		marked_ptr_t *move_dest_head = &h->new_b->head[grow_idx(old_idx)];
		marked_ptr_t *move_src_head = &h->b->head[old_idx];

		/* Head move not yet completed. */
		if (N_INVALID != get_mark(*move_src_head)) {
			complete_head_move(move_src_head, move_dest_head);
		}

		size_t split_idx = grow_to_split_idx(old_idx);
		size_t split_hash = calc_split_hash(split_idx, h->new_b->order);
		marked_ptr_t *split_dest_head = &h->new_b->head[split_idx];

		split_bucket(h, move_dest_head, split_dest_head, split_hash);
	}

	/*
	 * Wait for all updaters to notice the new heads. Once everyone sees
	 * the invalid old bucket heads they will know a resize is in progress
	 * and updaters will modify the correct new buckets.
	 */
	rcu_synchronize();

	/* Clear the JOIN_FOLLOWS mark and remove the link between the split buckets.*/
	for (size_t old_idx = 0; old_idx < old_bucket_cnt; ++old_idx) {
		size_t new_idx = grow_idx(old_idx);

		cleanup_join_follows(h, &h->new_b->head[new_idx]);
	}

	/*
	 * Wait for everyone to notice that buckets were split, ie link connecting
	 * the join follows and join node has been cut.
	 */
	rcu_synchronize();

	/* Clear the JOIN mark and GC any deleted join nodes. */
	for (size_t old_idx = 0; old_idx < old_bucket_cnt; ++old_idx) {
		size_t new_idx = grow_to_split_idx(old_idx);

		cleanup_join_node(h, &h->new_b->head[new_idx]);
	}

	/* Wait for everyone to see that the table is clear of any resize marks. */
	rcu_synchronize();

	cht_buckets_t *old_b = h->b;
	rcu_assign(h->b, h->new_b);

	/* Wait for everyone to start using the new table. */
	rcu_synchronize();

	free(old_b);

	/* Not needed; just for increased readability. */
	h->new_b = NULL;
}

/** Halfs the number of buckets. Blocks until done. */
static void shrink_table(cht_t *h)
{
	if (h->b->order <= h->min_order)
		return;

	h->new_b = alloc_buckets(h->b->order - 1, true, false);

	/* Failed to alloc a new table - try next time the resizer is run. */
	if (!h->new_b)
		return;

	/* Wait for all readers and updaters to see the initialized new table. */
	rcu_synchronize();

	size_t old_bucket_cnt = (1 << h->b->order);

	/*
	 * Give updaters a chance to help out with the resize. Do the minimum
	 * work needed to announce a resize is in progress, ie start moving heads.
	 */
	for (size_t old_idx = 0; old_idx < old_bucket_cnt; ++old_idx) {
		size_t new_idx = shrink_idx(old_idx);

		/* This bucket should be moved. */
		if (grow_idx(new_idx) == old_idx) {
			start_head_move(&h->b->head[old_idx]);
		} else {
			/* This bucket should join the moved bucket once the move is done.*/
		}
	}

	/* Order start_head_move() wrt to complete_head_move(). */
	cas_order_barrier();

	/* Complete moving heads and join buckets with the moved buckets. */
	for (size_t old_idx = 0; old_idx < old_bucket_cnt; ++old_idx) {
		size_t new_idx = shrink_idx(old_idx);
		size_t move_src_idx = grow_idx(new_idx);

		/* This bucket should be moved. */
		if (move_src_idx == old_idx) {
			/* Head move not yet completed. */
			if (N_INVALID != get_mark(h->b->head[old_idx])) {
				complete_head_move(&h->b->head[old_idx], &h->new_b->head[new_idx]);
			}
		} else {
			/* This bucket should join the moved bucket. */
			size_t split_hash = calc_split_hash(old_idx, h->b->order);
			join_buckets(h, &h->b->head[old_idx], &h->new_b->head[new_idx],
				split_hash);
		}
	}

	/*
	 * Wait for all updaters to notice the new heads. Once everyone sees
	 * the invalid old bucket heads they will know a resize is in progress
	 * and updaters will modify the correct new buckets.
	 */
	rcu_synchronize();

	/* Let everyone know joins are complete and fully visible. */
	for (size_t old_idx = 0; old_idx < old_bucket_cnt; ++old_idx) {
		size_t move_src_idx = grow_idx(shrink_idx(old_idx));

		/* Set the invalid joinee head to NULL. */
		if (old_idx != move_src_idx) {
			assert(N_INVALID == get_mark(h->b->head[old_idx]));

			if (&sentinel != get_next(h->b->head[old_idx]))
				h->b->head[old_idx] = make_link(&sentinel, N_INVALID);
		}
	}

	/* todo comment join node vs reset joinee head*/
	rcu_synchronize();

	size_t new_bucket_cnt = (1 << h->new_b->order);

	/* Clear the JOIN mark and GC any deleted join nodes. */
	for (size_t new_idx = 0; new_idx < new_bucket_cnt; ++new_idx) {
		cleanup_join_node(h, &h->new_b->head[new_idx]);
	}

	/* Wait for everyone to see that the table is clear of any resize marks. */
	rcu_synchronize();

	cht_buckets_t *old_b = h->b;
	rcu_assign(h->b, h->new_b);

	/* Wait for everyone to start using the new table. */
	rcu_synchronize();

	free(old_b);

	/* Not needed; just for increased readability. */
	h->new_b = NULL;
}

/** Finds and clears the N_JOIN mark from a node in new_head (if present). */
static void cleanup_join_node(cht_t *h, marked_ptr_t *new_head)
{
	rcu_read_lock();

	cht_link_t *cur = get_next(*new_head);

	while (cur != &sentinel) {
		/* Clear the join node's JN mark - even if it is marked as deleted. */
		if (N_JOIN & get_mark(cur->link)) {
			clear_join_and_gc(h, cur, new_head);
			break;
		}

		cur = get_next(cur->link);
	}

	rcu_read_unlock();
}

/** Clears the join_node's N_JOIN mark frees it if marked N_DELETED as well. */
static void clear_join_and_gc(cht_t *h, cht_link_t *join_node,
	marked_ptr_t *new_head)
{
	assert(join_node != &sentinel);
	assert(join_node && (N_JOIN & get_mark(join_node->link)));

	bool done;

	/* Clear the JN mark. */
	do {
		marked_ptr_t jn_link = join_node->link;
		cht_link_t *next = get_next(jn_link);
		/* Clear the JOIN mark but keep the DEL mark if present. */
		mark_t cleared_mark = get_mark(jn_link) & N_DELETED;

		marked_ptr_t ret =
			_cas_link(&join_node->link, jn_link, make_link(next, cleared_mark));

		/* Done if the mark was cleared. Retry if a new node was inserted. */
		done = (ret == jn_link);
		assert(ret == jn_link || (get_mark(ret) & N_JOIN));
	} while (!done);

	if (!(N_DELETED & get_mark(join_node->link)))
		return;

	/* The join node had been marked as deleted - GC it. */

	/* Clear the JOIN mark before trying to unlink the deleted join node.*/
	cas_order_barrier();

	size_t jn_hash = node_hash(h, join_node);
	do {
		bool resizing = false;

		wnd_t wnd = {
			.ppred = new_head,
			.cur = get_next(*new_head)
		};

		done = find_wnd_and_gc_pred(h, jn_hash, WM_NORMAL, same_node_pred,
			join_node, &wnd, &resizing);

		assert(!resizing);
	} while (!done);
}

/** Finds a non-deleted node with N_JOIN_FOLLOWS and clears the mark. */
static void cleanup_join_follows(cht_t *h, marked_ptr_t *new_head)
{
	assert(new_head);

	rcu_read_lock();

	wnd_t wnd = {
		.ppred = NULL,
		.cur = NULL
	};
	marked_ptr_t *cur_link = new_head;

	/*
	 * Find the non-deleted node with a JF mark and clear the JF mark.
	 * The JF node may be deleted and/or the mark moved to its neighbors
	 * at any time. Therefore, we GC deleted nodes until we find the JF
	 * node in order to remove stale/deleted JF nodes left behind eg by
	 * delayed threads that did not yet get a chance to unlink the deleted
	 * JF node and move its mark.
	 *
	 * Note that the head may be marked JF (but never DELETED).
	 */
	while (true) {
		bool is_jf_node = N_JOIN_FOLLOWS & get_mark(*cur_link);

		/* GC any deleted nodes on the way - even deleted JOIN_FOLLOWS. */
		if (N_DELETED & get_mark(*cur_link)) {
			assert(cur_link != new_head);
			assert(wnd.ppred && wnd.cur && wnd.cur != &sentinel);
			assert(cur_link == &wnd.cur->link);

			bool dummy;
			bool deleted = gc_deleted_node(h, WM_MOVE_JOIN_FOLLOWS, &wnd, &dummy);

			/* Failed to GC or collected a deleted JOIN_FOLLOWS. */
			if (!deleted || is_jf_node) {
				/* Retry from the head of the bucket. */
				cur_link = new_head;
				continue;
			}
		} else {
			/* Found a non-deleted JF. Clear its JF mark. */
			if (is_jf_node) {
				cht_link_t *next = get_next(*cur_link);
				marked_ptr_t ret =
					cas_link(cur_link, next, N_JOIN_FOLLOWS, &sentinel, N_NORMAL);

				assert(next == &sentinel
					|| ((N_JOIN | N_JOIN_FOLLOWS) & get_mark(ret)));

				/* Successfully cleared the JF mark of a non-deleted node. */
				if (ret == make_link(next, N_JOIN_FOLLOWS)) {
					break;
				} else {
					/*
					 * The JF node had been deleted or a new node inserted
					 * right after it. Retry from the head.
					 */
					cur_link = new_head;
					continue;
				}
			} else {
				wnd.ppred = cur_link;
				wnd.cur = get_next(*cur_link);
			}
		}

		/* We must encounter a JF node before we reach the end of the bucket. */
		assert(wnd.cur && wnd.cur != &sentinel);
		cur_link = &wnd.cur->link;
	}

	rcu_read_unlock();
}

/** Returns the first possible hash following a bucket split point.
 *
 * In other words the returned hash is the smallest possible hash
 * the remainder of the split bucket may contain.
 */
static inline size_t calc_split_hash(size_t split_idx, size_t order)
{
	assert(1 <= order && order <= 8 * sizeof(size_t));
	return split_idx << (8 * sizeof(size_t) - order);
}

/** Returns the bucket head index given the table size order and item hash. */
static inline size_t calc_bucket_idx(size_t hash, size_t order)
{
	assert(1 <= order && order <= 8 * sizeof(size_t));
	return hash >> (8 * sizeof(size_t) - order);
}

/** Returns the bucket index of destination*/
static inline size_t grow_to_split_idx(size_t old_idx)
{
	return grow_idx(old_idx) | 1;
}

/** Returns the destination index of a bucket head when the table is growing. */
static inline size_t grow_idx(size_t idx)
{
	return idx << 1;
}

/** Returns the destination index of a bucket head when the table is shrinking.*/
static inline size_t shrink_idx(size_t idx)
{
	return idx >> 1;
}

/** Returns a mixed hash of the search key.*/
static inline size_t calc_key_hash(cht_t *h, void *key)
{
	/* Mimic calc_node_hash. */
	return hash_mix(h->op->key_hash(key)) & ~(size_t)1;
}

/** Returns a memoized mixed hash of the item. */
static inline size_t node_hash(cht_t *h, const cht_link_t *item)
{
	assert(item->hash == h->invalid_hash
		|| item->hash == sentinel.hash
		|| item->hash == calc_node_hash(h, item));

	return item->hash;
}

/** Calculates and mixed the hash of the item. */
static inline size_t calc_node_hash(cht_t *h, const cht_link_t *item)
{
	assert(item != &sentinel);
	/*
	 * Clear the lowest order bit in order for sentinel's node hash
	 * to be the greatest possible.
	 */
	return hash_mix(h->op->hash(item)) & ~(size_t)1;
}

/** Computes and memoizes the hash of the item. */
static inline void memoize_node_hash(cht_t *h, cht_link_t *item)
{
	item->hash = calc_node_hash(h, item);
}

/** Packs the next pointer address and the mark into a single pointer. */
static inline marked_ptr_t make_link(const cht_link_t *next, mark_t mark)
{
	marked_ptr_t ptr = (marked_ptr_t) next;

	assert(!(ptr & N_MARK_MASK));
	assert(!((unsigned)mark & ~N_MARK_MASK));

	return ptr | mark;
}

/** Strips any marks from the next item link and returns the next item's address.*/
static inline cht_link_t * get_next(marked_ptr_t link)
{
	return (cht_link_t*)(link & ~N_MARK_MASK);
}

/** Returns the current node's mark stored in the next item link. */
static inline mark_t get_mark(marked_ptr_t link)
{
	return (mark_t)(link & N_MARK_MASK);
}

/** Moves the window by one item so that is points to the next item. */
static inline void next_wnd(wnd_t *wnd)
{
	assert(wnd);
	assert(wnd->cur);

	wnd->last = wnd->cur;
	wnd->ppred = &wnd->cur->link;
	wnd->cur = get_next(wnd->cur->link);
}

/** Predicate that matches only exactly the same node. */
static bool same_node_pred(void *node, const cht_link_t *item2)
{
	const cht_link_t *item1 = (const cht_link_t*) node;
	return item1 == item2;
}

/** Compare-and-swaps a next item link. */
static inline marked_ptr_t cas_link(marked_ptr_t *link, const cht_link_t *cur_next,
	mark_t cur_mark, const cht_link_t *new_next, mark_t new_mark)
{
	return _cas_link(link, make_link(cur_next, cur_mark),
		make_link(new_next, new_mark));
}

/** Compare-and-swaps a next item link. */
static inline marked_ptr_t _cas_link(marked_ptr_t *link, marked_ptr_t cur,
	marked_ptr_t new)
{
	assert(link != &sentinel.link);
	/*
	 * cas(x) on the same location x on one cpu must be ordered, but do not
	 * have to be ordered wrt to other cas(y) to a different location y
	 * on the same cpu.
	 *
	 * cas(x) must act as a write barrier on x, ie if cas(x) succeeds
	 * and is observed by another cpu, then all cpus must be able to
	 * make the effects of cas(x) visible just by issuing a load barrier.
	 * For example:
	 * cpu1         cpu2            cpu3
	 *                              cas(x, 0 -> 1), succeeds
	 *              cas(x, 0 -> 1), fails
	 *              MB, to order load of x in cas and store to y
	 *              y = 7
	 * sees y == 7
	 * loadMB must be enough to make cas(x) on cpu3 visible to cpu1, ie x == 1.
	 *
	 * If cas() did not work this way:
	 * a) our head move protocol would not be correct.
	 * b) freeing an item linked to a moved head after another item was
	 *   inserted in front of it, would require more than one grace period.
	 *
	 * Ad (a): In the following example, cpu1 starts moving old_head
	 * to new_head, cpu2 completes the move and cpu3 notices cpu2
	 * completed the move before cpu1 gets a chance to notice cpu2
	 * had already completed the move. Our requirements for cas()
	 * assume cpu3 will see a valid and mutable value in new_head
	 * after issuing a load memory barrier once it has determined
	 * the old_head's value had been successfully moved to new_head
	 * (because it sees old_head marked invalid).
	 *
	 *  cpu1             cpu2             cpu3
	 *   cas(old_head, <addr, N>, <addr, Const>), succeeds
	 *   cas-order-barrier
	 *   // Move from old_head to new_head started, now the interesting stuff:
	 *   cas(new_head, <0, Inv>, <addr, N>), succeeds
	 *
	 *                    cas(new_head, <0, Inv>, <addr, N>), but fails
	 *                    cas-order-barrier
	 *                    cas(old_head, <addr, Const>, <addr, Inv>), succeeds
	 *
	 *                                     Sees old_head marked Inv (by cpu2)
	 *                                     load-MB
	 *                                     assert(new_head == <addr, N>)
	 *
	 *   cas-order-barrier
	 *
	 * Even though cpu1 did not yet issue a cas-order-barrier, cpu1's store
	 * to new_head (successful cas()) must be made visible to cpu3 with
	 * a load memory barrier if cpu1's store to new_head is visible
	 * on another cpu (cpu2) and that cpu's (cpu2's) store to old_head
	 * is already visible to cpu3.	 *
	 */
	void *expected = (void*)cur;

	/*
	 * Use the acquire-release model, although we could probably
	 * get away even with the relaxed memory model due to our use
	 * of explicit memory barriers.
	 */
	__atomic_compare_exchange_n((void**)link, &expected, (void *)new, false,
		__ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);

	return (marked_ptr_t) expected;
}

/** Orders compare-and-swaps to different memory locations. */
static inline void cas_order_barrier(void)
{
	/* Make sure CAS to different memory locations are ordered. */
	write_barrier();
}


/** @}
 */
