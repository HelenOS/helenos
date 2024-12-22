/*
 * Copyright (c) 2024 Miroslav Cimerman
 * Copyright (c) 2024 Vojtech Horky
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 * @brief Fibril group executor
 *
 * Fibril pool with pre-allocated storage allowing
 * execution of groups consisting of multiple work
 * units.
 */

#include <adt/bitmap.h>
#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <types/common.h>

#include "fge.h"

struct fge_fibril_data;
typedef struct fge_fibril_data fge_fibril_data_t;
struct wu_queue;
typedef struct wu_queue wu_queue_t;

static void *hr_fpool_make_storage(hr_fpool_t *, ssize_t *);
static errno_t fge_fibril(void *);
static errno_t wu_queue_init(wu_queue_t *, size_t);
static void wu_queue_push(wu_queue_t *, fge_fibril_data_t);
static fge_fibril_data_t wu_queue_pop(wu_queue_t *);
static ssize_t hr_fpool_get_free_slot(hr_fpool_t *);

typedef struct fge_fibril_data {
	hr_wu_t wu; /* user-provided work unit fcn pointer */
	void *arg;
	hr_fgroup_t *group;
	ssize_t memslot; /* index to pool bitmap slot */
} fge_fibril_data_t;

typedef struct wu_queue {
	fibril_mutex_t lock;
	fibril_condvar_t not_empty;
	fibril_condvar_t not_full;
	fge_fibril_data_t *fexecs;
	size_t capacity;
	size_t size;
	size_t front;
	size_t back;
} wu_queue_t;

struct hr_fpool {
	fibril_mutex_t lock;
	fibril_condvar_t all_wus_done;
	bitmap_t bitmap;
	wu_queue_t queue;
	fid_t *fibrils;
	uint8_t *wu_storage;
	size_t fibril_cnt;
	size_t max_wus;
	size_t active_groups;
	bool stop;
	size_t wu_size;
	size_t wu_storage_free_count;
};

struct hr_fgroup {
	hr_fpool_t *pool;
	size_t wu_cnt;		/* total wu count */
	size_t submitted;
	size_t reserved_cnt;	/* no. of reserved wu storage slots */
	size_t reserved_avail;
	size_t *memslots;	/* indices to pool bitmap */
	void *own_mem;
	size_t own_used;
	errno_t final_errno;
	atomic_size_t finished_okay;
	atomic_size_t finished_fail;
	atomic_size_t wus_started;
	fibril_mutex_t lock;
	fibril_condvar_t all_done;
};

hr_fpool_t *hr_fpool_create(size_t fibril_cnt, size_t max_wus,
    size_t wu_storage_size)
{
	void *bitmap_data = NULL;

	hr_fpool_t *result = calloc(1, sizeof(hr_fpool_t));
	if (result == NULL)
		return NULL;

	result->fibrils = malloc(sizeof(fid_t) * fibril_cnt);
	if (result->fibrils == NULL)
		goto bad;

	result->wu_storage = malloc(wu_storage_size * max_wus);
	if (result->wu_storage == NULL)
		goto bad;

	bitmap_data = calloc(1, bitmap_size(max_wus));
	if (bitmap_data == NULL)
		goto bad;
	bitmap_initialize(&result->bitmap, max_wus, bitmap_data);

	if (wu_queue_init(&result->queue, max_wus) != EOK)
		goto bad;

	fibril_mutex_initialize(&result->lock);
	fibril_condvar_initialize(&result->all_wus_done);

	result->max_wus = max_wus;
	result->fibril_cnt = fibril_cnt;
	result->wu_size = wu_storage_size;
	result->wu_storage_free_count = max_wus;
	result->stop = false;
	result->active_groups = 0;

	for (size_t i = 0; i < fibril_cnt; i++) {
		result->fibrils[i] = fibril_create(fge_fibril, result);
		fibril_start(result->fibrils[i]);
	}

	return result;
bad:
	if (result->queue.fexecs)
		free(result->queue.fexecs);
	if (bitmap_data)
		free(bitmap_data);
	if (result->wu_storage)
		free(result->wu_storage);
	if (result->fibrils)
		free(result->fibrils);
	free(result);

	return NULL;
}

void hr_fpool_destroy(hr_fpool_t *pool)
{
	fibril_mutex_lock(&pool->lock);
	pool->stop = true;
	while (pool->active_groups > 0)
		fibril_condvar_wait(&pool->all_wus_done, &pool->lock);

	fibril_mutex_unlock(&pool->lock);

	free(pool->bitmap.bits);
	free(pool->queue.fexecs);
	free(pool->wu_storage);
	free(pool->fibrils);
	free(pool);
}

static void *hr_fpool_make_storage(hr_fpool_t *pool, ssize_t *rmemslot)
{
	fibril_mutex_lock(&pool->lock);
	ssize_t memslot = hr_fpool_get_free_slot(pool);
	assert(memslot != -1);

	bitmap_set(&pool->bitmap, memslot, 1);

	fibril_mutex_unlock(&pool->lock);

	if (rmemslot)
		*rmemslot = memslot;

	return pool->wu_storage + pool->wu_size * memslot;
}

hr_fgroup_t *hr_fgroup_create(hr_fpool_t *parent, size_t wu_cnt)
{
	hr_fgroup_t *result = malloc(sizeof(hr_fgroup_t));
	if (result == NULL)
		return NULL;

	result->reserved_cnt = 0;
	result->own_mem = NULL;
	result->memslots = NULL;

	fibril_mutex_lock(&parent->lock);

	parent->active_groups++;

	if (parent->wu_storage_free_count >= wu_cnt) {
		parent->wu_storage_free_count -= wu_cnt;
		result->reserved_cnt = wu_cnt;
	} else {
		/*
		 * Could be more conservative with memory here and
		 * allocate space only for one work unit and execute
		 * work units sequentially like it was first intended with
		 * the fallback storage.
		 */
		size_t taking = parent->wu_storage_free_count;
		result->own_mem = malloc(parent->wu_size * (wu_cnt - taking));
		result->reserved_cnt = taking;
		parent->wu_storage_free_count = 0;
		if (result->own_mem == NULL)
			goto bad;
	}

	if (result->reserved_cnt > 0) {
		result->memslots =
		    malloc(sizeof(size_t) * result->reserved_cnt);
		if (result->memslots == NULL)
			goto bad;
	}

	fibril_mutex_unlock(&parent->lock);

	result->pool = parent;
	result->wu_cnt = wu_cnt;
	result->submitted = 0;
	result->reserved_avail = result->reserved_cnt;
	result->own_used = 0;
	result->final_errno = EOK;
	result->finished_okay = 0;
	result->finished_fail = 0;
	result->wus_started = 0;

	fibril_mutex_initialize(&result->lock);
	fibril_condvar_initialize(&result->all_done);

	return result;

bad:
	fibril_mutex_lock(&parent->lock);
	parent->wu_storage_free_count += result->reserved_cnt;
	fibril_mutex_unlock(&parent->lock);

	if (result->memslots)
		free(result->memslots);
	if (result->own_mem)
		free(result->own_mem);
	free(result);

	return NULL;
}

void *hr_fgroup_alloc(hr_fgroup_t *group)
{
	void *storage;

	fibril_mutex_lock(&group->lock);

	if (group->reserved_avail > 0) {
		ssize_t memslot;
		storage = hr_fpool_make_storage(group->pool, &memslot);
		assert(storage != NULL);
		group->reserved_avail--;
		group->memslots[group->submitted] = memslot;
	} else {
		storage =
		    group->own_mem + group->pool->wu_size * group->own_used;
		group->own_used++;
	}

	fibril_mutex_unlock(&group->lock);

	return storage;
}

void hr_fgroup_submit(hr_fgroup_t *group, hr_wu_t wu, void *arg)
{
	fibril_mutex_lock(&group->lock);
	assert(group->submitted + 1 <= group->wu_cnt);

	fge_fibril_data_t executor;
	executor.wu = wu;
	executor.arg = arg;
	executor.group = group;

	if (group->submitted < group->reserved_cnt)
		executor.memslot = group->memslots[group->submitted];
	else
		executor.memslot = -1;

	group->submitted++;
	fibril_mutex_unlock(&group->lock);

	wu_queue_push(&group->pool->queue, executor);
}

static void hr_fpool_group_epilogue(hr_fpool_t *pool)
{
	fibril_mutex_lock(&pool->lock);

	pool->active_groups--;
	if (pool->active_groups == 0)
		fibril_condvar_signal(&pool->all_wus_done);

	fibril_mutex_unlock(&pool->lock);
}

errno_t hr_fgroup_wait(hr_fgroup_t *group, size_t *rokay, size_t *rfailed)
{
	fibril_mutex_lock(&group->lock);
	while (true) {
		size_t finished = group->finished_fail + group->finished_okay;
		if (group->wus_started != 0 && group->wus_started == finished)
			break;

		fibril_condvar_wait(&group->all_done, &group->lock);
	}

	if (rokay)
		*rokay = group->finished_okay;
	if (rfailed)
		*rfailed = group->finished_fail;

	errno_t rc = EOK;
	if (group->finished_okay != group->wus_started)
		rc = EIO;

	fibril_mutex_unlock(&group->lock);

	hr_fpool_group_epilogue(group->pool);

	if (group->memslots)
		free(group->memslots);
	if (group->own_mem)
		free(group->own_mem);
	free(group);

	return rc;
}

static errno_t fge_fibril(void *arg)
{
	hr_fpool_t *pool = arg;
	while (true) {
		fge_fibril_data_t executor;
		fibril_mutex_lock(&pool->lock);

		while (pool->queue.size == 0 && !pool->stop) {
			fibril_condvar_wait(&pool->queue.not_empty,
			    &pool->lock);
		}

		if (pool->stop && pool->queue.size == 0) {
			fibril_mutex_unlock(&pool->lock);
			break;
		}

		executor = wu_queue_pop(&pool->queue);

		fibril_mutex_unlock(&pool->lock);

		hr_fgroup_t *group = executor.group;

		atomic_fetch_add_explicit(&group->wus_started, 1,
		    memory_order_relaxed);

		errno_t rc = executor.wu(executor.arg);

		if (rc == EOK)
			atomic_fetch_add_explicit(&group->finished_okay, 1,
			    memory_order_relaxed);
		else
			atomic_fetch_add_explicit(&group->finished_fail, 1,
			    memory_order_relaxed);

		fibril_mutex_lock(&pool->lock);
		if (executor.memslot > -1) {
			bitmap_set(&pool->bitmap, executor.memslot, 0);
			pool->wu_storage_free_count++;
		}

		size_t group_total_done = group->finished_fail +
		    group->finished_okay;
		if (group->wus_started == group_total_done)
			fibril_condvar_signal(&group->all_done);

		fibril_mutex_unlock(&pool->lock);
	}
	return EOK;
}

static errno_t wu_queue_init(wu_queue_t *queue, size_t capacity)
{
	queue->fexecs = malloc(sizeof(fge_fibril_data_t) * capacity);
	if (queue->fexecs == NULL)
		return ENOMEM;

	queue->capacity = capacity;
	queue->size = 0;
	queue->front = 0;
	queue->back = 0;
	fibril_mutex_initialize(&queue->lock);
	fibril_condvar_initialize(&queue->not_empty);
	fibril_condvar_initialize(&queue->not_full);

	return EOK;
}

static void wu_queue_push(wu_queue_t *queue, fge_fibril_data_t executor)
{
	fibril_mutex_lock(&queue->lock);

	while (queue->size == queue->capacity)
		fibril_condvar_wait(&queue->not_full, &queue->lock);

	queue->fexecs[queue->back] = executor;
	queue->back = (queue->back + 1) % queue->capacity;
	queue->size++;

	fibril_condvar_signal(&queue->not_empty);

	fibril_mutex_unlock(&queue->lock);
}

static fge_fibril_data_t wu_queue_pop(wu_queue_t *queue)
{
	fibril_mutex_lock(&queue->lock);

	while (queue->size == 0)
		fibril_condvar_wait(&queue->not_empty, &queue->lock);

	fge_fibril_data_t wu = queue->fexecs[queue->front];
	queue->front = (queue->front + 1) % queue->capacity;
	queue->size--;

	fibril_condvar_signal(&queue->not_full);

	fibril_mutex_unlock(&queue->lock);
	return wu;
}

static ssize_t hr_fpool_get_free_slot(hr_fpool_t *pool)
{
	bitmap_t *bitmap = &pool->bitmap;
	for (size_t i = 0; i < pool->max_wus; i++)
		if (!bitmap_get(bitmap, i))
			return i;
	return -1;
}

/** @}
 */
