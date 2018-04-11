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

#include <assert.h>
#include <test.h>
#include <print.h>
#include <adt/cht.h>
#include <synch/rcu.h>

typedef struct val {
	/* Place at the top to simplify re-casting. */
	cht_link_t link;
	size_t hash;
	size_t unique_id;
	bool deleted;
	bool mark;
} val_t;

static size_t val_hash(const cht_link_t *item)
{
	val_t *v = member_to_inst(item, val_t, link);
	assert(v->hash == (v->unique_id % 10));
	return v->hash;
}

static size_t val_key_hash(void *key)
{
	return (uintptr_t)key % 10;
}

static bool val_equal(const cht_link_t *item1, const cht_link_t *item2)
{
	val_t *v1 = member_to_inst(item1, val_t, link);
	val_t *v2 = member_to_inst(item2, val_t, link);
	return v1->unique_id == v2->unique_id;
}

static bool val_key_equal(void *key, const cht_link_t *item2)
{
	val_t *v2 = member_to_inst(item2, val_t, link);
	return (uintptr_t)key == v2->unique_id;
}

static void val_rm_callback(cht_link_t *item)
{
	val_t *v = member_to_inst(item, val_t, link);
	assert(!v->deleted);
	v->deleted = true;
	free(v);
}


static cht_ops_t val_ops = {
	.hash = val_hash,
	.key_hash = val_key_hash,
	.equal = val_equal,
	.key_equal = val_key_equal,
	.remove_callback = val_rm_callback,
};

static void set_val(val_t *v, size_t h, size_t uid)
{
	v->hash = h;
	v->unique_id = uid;
	v->deleted = false;
	v->mark = false;
}

/*-------------------------------------------------------------------*/


static const char *do_sanity_test(cht_t *h)
{
	if (cht_find_lazy(h, (void *)0))
		return "Found lazy in empty table.";

	if (cht_find(h, (void *)0))
		return "Found in empty table.";

	if (cht_remove_key(h, (void *)0))
		return "Removed from empty table.";

	const int val_cnt = 6;
	val_t *v[6] = { NULL };

	for (int i = 0; i < val_cnt; ++i)
		v[i] = malloc(sizeof(val_t), 0);

	size_t key[] = { 1, 1, 1, 11, 12, 13 };

	/* First three are identical */
	for (int i = 0; i < 3; ++i)
		set_val(v[i], 1, key[i]);

	/* Same hash, different key.*/
	set_val(v[3], 1, key[3]);

	/* Different hashes and keys. */
	set_val(v[4], 2, key[4]);
	set_val(v[5], 3, key[5]);

	cht_link_t *dup;

	if (!cht_insert_unique(h, &v[0]->link, &dup))
		return "Duplicates in empty";

	if (cht_insert_unique(h, &v[1]->link, &dup))
		return "Inserted a duplicate";

	if (dup != &v[0]->link)
		return "Returned wrong duplicate";

	if (!cht_insert_unique(h, &v[3]->link, &dup))
		return "Refused non-equal item but with a hash in table.";

	cht_insert(h, &v[1]->link);
	cht_insert(h, &v[2]->link);

	bool ok = true;
	ok = ok && cht_insert_unique(h, &v[4]->link, &dup);
	ok = ok && cht_insert_unique(h, &v[5]->link, &dup);

	if (!ok)
		return "Refused unique ins 4, 5.";

	if (cht_find(h, (void *)0))
		return "Phantom find.";

	cht_link_t *item = cht_find(h, (void *)v[5]->unique_id);
	if (!item || item != &v[5]->link)
		return "Missing 5.";

	item = cht_find_next(h, &v[5]->link);
	if (item)
		return "Found nonexisting duplicate 5";

	item = cht_find(h, (void *)v[3]->unique_id);
	if (!item || item != &v[3]->link)
		return "Missing 3.";

	item = cht_find_next(h, &v[3]->link);
	if (item)
		return "Found nonexisting duplicate 3, same hash as others.";

	item = cht_find(h, (void *)v[0]->unique_id);
	((val_t *)item)->mark = true;

	for (int k = 1; k < 3; ++k) {
		item = cht_find_next(h, item);
		if (!item)
			return "Did not find an inserted duplicate";

		val_t *val = ((val_t *)item);

		if (val->unique_id != v[0]->unique_id)
			return "Found item with a different key.";
		if (val->mark)
			return "Found twice the same node.";
		val->mark = true;
	}

	for (int i = 0; i < 3; ++i) {
		if (!v[i]->mark)
			return "Did not find all duplicates";

		v[i]->mark = false;
	}

	if (cht_find_next(h, item))
		return "Found non-existing duplicate.";

	item = cht_find_next(h, cht_find(h, (void *)key[0]));

	((val_t *)item)->mark = true;
	if (!cht_remove_item(h, item))
		return "Failed to remove inserted item";

	item = cht_find(h, (void *)key[0]);
	if (!item || ((val_t *)item)->mark)
		return "Did not find proper item.";

	item = cht_find_next(h, item);
	if (!item || ((val_t *)item)->mark)
		return "Did not find proper duplicate.";

	item = cht_find_next(h, item);
	if (item)
		return "Found removed duplicate";

	if (2 != cht_remove_key(h, (void *)key[0]))
		return "Failed to remove all duplicates";

	if (cht_find(h, (void *)key[0]))
		return "Found removed key";

	if (!cht_find(h, (void *)key[3]))
		return "Removed incorrect key";

	for (size_t k = 0; k < sizeof(v) / sizeof(v[0]); ++k) {
		cht_remove_key(h, (void *)key[k]);
	}

	for (size_t k = 0; k < sizeof(v) / sizeof(v[0]); ++k) {
		if (cht_find(h, (void *)key[k]))
			return "Found a key in a cleared table";
	}

	return NULL;
}

static const char *sanity_test(void)
{
	cht_t h;
	if (!cht_create_simple(&h, &val_ops))
		return "Could not create the table.";

	rcu_read_lock();
	const char *err = do_sanity_test(&h);
	rcu_read_unlock();

	cht_destroy(&h);

	return err;
}

/*-------------------------------------------------------------------*/

static size_t next_rand(size_t seed)
{
	return (seed * 1103515245 + 12345) & ((1U << 31) - 1);
}

/*-------------------------------------------------------------------*/
typedef struct {
	cht_link_t link;
	size_t key;
	bool free;
	bool inserted;
	bool deleted;
} stress_t;

typedef struct {
	cht_t *h;
	int *stop;
	stress_t *elem;
	size_t elem_cnt;
	size_t upd_prob;
	size_t wave_cnt;
	size_t wave_elems;
	size_t id;
	bool failed;
} stress_work_t;

static size_t stress_hash(const cht_link_t *item)
{
	return ((stress_t *)item)->key >> 8;
}
static size_t stress_key_hash(void *key)
{
	return ((size_t)key) >> 8;
}
static bool stress_equal(const cht_link_t *item1, const cht_link_t *item2)
{
	return ((stress_t *)item1)->key == ((stress_t *)item2)->key;
}
static bool stress_key_equal(void *key, const cht_link_t *item)
{
	return ((size_t)key) == ((stress_t *)item)->key;
}
static void stress_rm_callback(cht_link_t *item)
{
	if (((stress_t *)item)->free)
		free(item);
	else
		((stress_t *)item)->deleted = true;
}

cht_ops_t stress_ops = {
	.hash = stress_hash,
	.key_hash = stress_key_hash,
	.equal = stress_equal,
	.key_equal = stress_key_equal,
	.remove_callback = stress_rm_callback
};

static void resize_stresser(void *arg)
{
	stress_work_t *work = (stress_work_t *)arg;

	for (size_t k = 0; k < work->wave_cnt; ++k) {
		TPRINTF("I{");
		for (size_t i = 0; i < work->wave_elems; ++i) {
			stress_t *s = malloc(sizeof(stress_t), FRAME_ATOMIC);
			if (!s) {
				TPRINTF("[out-of-mem]\n");
				goto out_of_mem;
			}

			s->free = true;
			s->key = (i << 8) + work->id;

			cht_insert(work->h, &s->link);
		}
		TPRINTF("}");

		thread_sleep(2);

		TPRINTF("R<");
		for (size_t i = 0; i < work->wave_elems; ++i) {
			size_t key = (i << 8) + work->id;

			if (1 != cht_remove_key(work->h, (void *)key)) {
				TPRINTF("Err: Failed to remove inserted item\n");
				goto failed;
			}
		}
		TPRINTF(">");
	}

	/* Request that others stop. */
	*work->stop = 1;
	return;

failed:
	work->failed = true;

out_of_mem:
	/* Request that others stop. */
	*work->stop = 1;

	/* Remove anything we may have inserted. */
	for (size_t i = 0; i < work->wave_elems; ++i) {
		size_t key = (i << 8) + work->id;
		cht_remove_key(work->h, (void *)key);
	}
}

static void op_stresser(void *arg)
{
	stress_work_t *work = (stress_work_t *)arg;
	assert(0 == *work->stop);

	size_t loops = 0;
	size_t seed = work->id;

	while (0 == *work->stop && !work->failed) {
		seed = next_rand(seed);
		bool upd = ((seed % 100) <= work->upd_prob);
		seed = next_rand(seed);
		size_t elem_idx = seed % work->elem_cnt;

		++loops;
		if (0 == loops % (1024 * 1024)) {
			/* Make the most current work->stop visible. */
			read_barrier();
			TPRINTF("*");
		}

		if (upd) {
			seed = next_rand(seed);
			bool item_op = seed & 1;

			if (work->elem[elem_idx].inserted) {
				if (item_op) {
					rcu_read_lock();
					cht_remove_item(work->h, &work->elem[elem_idx].link);
					rcu_read_unlock();
				} else {
					void *key = (void *)work->elem[elem_idx].key;
					if (1 != cht_remove_key(work->h, key)) {
						TPRINTF("Err: did not rm the key\n");
						work->failed = true;
					}
				}
				work->elem[elem_idx].inserted = false;
			} else if (work->elem[elem_idx].deleted) {
				work->elem[elem_idx].deleted = false;

				if (item_op) {
					rcu_read_lock();
					cht_link_t *dup;
					if (!cht_insert_unique(work->h, &work->elem[elem_idx].link,
					    &dup)) {
						TPRINTF("Err: already inserted\n");
						work->failed = true;
					}
					rcu_read_unlock();
				} else {
					cht_insert(work->h, &work->elem[elem_idx].link);
				}

				work->elem[elem_idx].inserted = true;
			}
		} else {
			rcu_read_lock();
			cht_link_t *item =
			    cht_find(work->h, (void *)work->elem[elem_idx].key);
			rcu_read_unlock();

			if (item) {
				if (!work->elem[elem_idx].inserted) {
					TPRINTF("Err: found but not inserted!");
					work->failed = true;
				}
				if (item != &work->elem[elem_idx].link) {
					TPRINTF("Err: found but incorrect item\n");
					work->failed = true;
				}
			} else {
				if (work->elem[elem_idx].inserted) {
					TPRINTF("Err: inserted but not found!");
					work->failed = true;
				}
			}
		}
	}


	/* Remove anything we may have inserted. */
	for (size_t i = 0; i < work->elem_cnt; ++i) {
		void *key = (void *) work->elem[i].key;
		cht_remove_key(work->h, key);
	}
}

static bool do_stress(void)
{
	cht_t h;

	if (!cht_create_simple(&h, &stress_ops)) {
		TPRINTF("Failed to create the table\n");
		return false;
	}

	const size_t wave_cnt = 10;
	const size_t max_thread_cnt = 8;
	const size_t resize_thread_cnt = 2;
	size_t op_thread_cnt = min(max_thread_cnt, 2 * config.cpu_active);
	size_t total_thr_cnt = op_thread_cnt + resize_thread_cnt;
	size_t items_per_thread = 1024;

	size_t work_cnt = op_thread_cnt + resize_thread_cnt;
	size_t item_cnt = op_thread_cnt * items_per_thread;

	/* Alloc hash table items. */
	size_t size = item_cnt * sizeof(stress_t) + work_cnt * sizeof(stress_work_t) +
	    sizeof(int);

	TPRINTF("Alloc and init table items. \n");
	void *p = malloc(size, FRAME_ATOMIC);
	if (!p) {
		TPRINTF("Failed to alloc items\n");
		cht_destroy(&h);
		return false;
	}

	stress_t *pitem = p + work_cnt * sizeof(stress_work_t);
	stress_work_t *pwork = p;
	int *pstop = (int *)(pitem + item_cnt);

	*pstop = 0;

	/* Init work items. */
	for (size_t i = 0; i < op_thread_cnt; ++i) {
		pwork[i].h = &h;
		pwork[i].stop = pstop;
		pwork[i].elem = &pitem[i * items_per_thread];
		pwork[i].upd_prob = (i + 1) * 100 / op_thread_cnt;
		pwork[i].id = i;
		pwork[i].elem_cnt = items_per_thread;
		pwork[i].failed = false;
	}

	for (size_t i = op_thread_cnt; i < op_thread_cnt + resize_thread_cnt; ++i) {
		pwork[i].h = &h;
		pwork[i].stop = pstop;
		pwork[i].wave_cnt = wave_cnt;
		pwork[i].wave_elems = item_cnt * 4;
		pwork[i].id = i;
		pwork[i].failed = false;
	}

	/* Init table elements. */
	for (size_t k = 0; k < op_thread_cnt; ++k) {
		for (size_t i = 0; i < items_per_thread; ++i) {
			pwork[k].elem[i].key = (i << 8) + k;
			pwork[k].elem[i].free = false;
			pwork[k].elem[i].inserted = false;
			pwork[k].elem[i].deleted = true;
		}
	}

	TPRINTF("Running %zu ins/del/find stress threads + %zu resizers.\n",
	    op_thread_cnt, resize_thread_cnt);

	/* Create and run threads. */
	thread_t *thr[max_thread_cnt + resize_thread_cnt];

	for (size_t i = 0; i < total_thr_cnt; ++i) {
		if (i < op_thread_cnt)
			thr[i] = thread_create(op_stresser, &pwork[i], TASK, 0, "cht-op-stress");
		else
			thr[i] = thread_create(resize_stresser, &pwork[i], TASK, 0, "cht-resize");

		assert(thr[i]);
		thread_wire(thr[i], &cpus[i % config.cpu_active]);
		thread_ready(thr[i]);
	}

	bool failed = false;

	/* Wait for all threads to return. */
	TPRINTF("Joining resize stressers.\n");
	for (size_t i = op_thread_cnt; i < total_thr_cnt; ++i) {
		thread_join(thr[i]);
		thread_detach(thr[i]);
		failed = pwork[i].failed || failed;
	}

	TPRINTF("Joining op stressers.\n");
	for (int i = (int)op_thread_cnt - 1; i >= 0; --i) {
		TPRINTF("%d threads remain\n", i);
		thread_join(thr[i]);
		thread_detach(thr[i]);
		failed = pwork[i].failed || failed;
	}

	cht_destroy(&h);
	free(p);

	return !failed;
}

/*-------------------------------------------------------------------*/


const char *test_cht1(void)
{
	const char *err = sanity_test();
	if (err)
		return err;
	printf("Basic sanity test: ok.\n");

	if (!do_stress())
		return "CHT stress test failed.";
	else
		return NULL;
}
