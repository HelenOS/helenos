/*
 * Copyright (c) 2009 Martin Decky
 * Copyright (c) 2009 Jiri Svoboda
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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
#include <async.h>
#include <errno.h>
#include <macros.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <task.h>
#include <types/task.h>

#include "task.h"
#include "taskman.h"

typedef struct {
	task_walker_t walker;
	void *arg;
} walker_context_t;

/*
 * Forwards
 */

static void task_destroy(task_t **);

/*
 * Hash table functions
 */

static size_t ht_task_key_hash(const void *key)
{
	return *(task_id_t *)key;
}

static size_t ht_task_hash(const ht_link_t  *item)
{
	task_t *ht = hash_table_get_inst(item, task_t, link);
	return ht->id;
}

static bool ht_task_key_equal(const void *key, const ht_link_t *item)
{
	task_t *ht = hash_table_get_inst(item, task_t, link);
	return ht->id == *(task_id_t *)key;
}

/** Perform actions after removal of item from the hash table. */
static void ht_task_remove(ht_link_t *item)
{
	task_t *t = hash_table_get_inst(item, task_t, link);
	task_destroy(&t);
}

/** Operations for task hash table. */
static hash_table_ops_t task_hash_table_ops = {
	.hash = ht_task_hash,
	.key_hash = ht_task_key_hash,
	.key_equal = ht_task_key_equal,
	.equal = NULL,
	.remove_callback = ht_task_remove
};

/** Task hash table structure. */
static hash_table_t task_hash_table;
fibril_rwlock_t task_hash_table_lock;

static void task_init(task_t *t)
{
	t->exit = TASK_EXIT_RUNNING;
	t->failed = false;
	t->retval_type = RVAL_UNSET;
	t->retval = -1;
	link_initialize(&t->listeners);
	t->sess = NULL;
}

static void task_destroy(task_t **t_ptr)
{
	task_t *t = *t_ptr;
	if (t == NULL) {
		return;
	}

	if (t->sess != NULL) {
		async_hangup(t->sess);
	}
	free(t);

	*t_ptr = NULL;
}

errno_t tasks_init(void)
{
	if (!hash_table_create(&task_hash_table, 0, 0, &task_hash_table_ops)) {
		printf(NAME ": No memory available for tasks\n");
		return ENOMEM;
	}

	fibril_rwlock_initialize(&task_hash_table_lock);

	return EOK;
}

/** Find task by its ID
 *
 * Assumes held lock of task_hash_table.
 *
 * @param[in]  id
 * @return task structure
 * @return NULL when no task with given ID exists
 */
task_t *task_get_by_id(task_id_t id)
{
	ht_link_t *link = hash_table_find(&task_hash_table, &id);
	if (!link) {
		return NULL;
	}

	task_t *t = hash_table_get_inst(link, task_t, link);
	return t;
}

static bool internal_walker(ht_link_t *ht_link, void *arg)
{
	task_t *t = hash_table_get_inst(ht_link, task_t, link);
	walker_context_t *ctx = arg;
	return ctx->walker(t, ctx->arg);
}

/** Iterate over all tasks
 *
 * @note Assumes task_hash_table lock is held.
 *
 * @param[in]  walker
 * @param[in]  arg     generic argument passed to walker function
 */
void task_foreach(task_walker_t walker, void *arg)
{
	walker_context_t ctx;
	ctx.walker = walker;
	ctx.arg = arg;

	hash_table_apply(&task_hash_table, &internal_walker, &ctx);
}

/** Remove task from our structures
 *
 * @note Assumes task_hash_table exclusive lock is held.
 *
 * @param[in|out]  ptr_t  Pointer to task pointer that should be removed, nulls
 *                        task pointer.
 */
void task_remove(task_t **ptr_t)
{
	task_t *t = *ptr_t;
	if (t == NULL) {
		return;
	}

	hash_table_remove_item(&task_hash_table, &t->link);
	*ptr_t = NULL;
}

errno_t task_intro(task_id_t id)
{
	errno_t rc = EOK;

	fibril_rwlock_write_lock(&task_hash_table_lock);

	task_t *t = task_get_by_id(id);
	if (t != NULL) {
		rc = EEXIST;
		goto finish;
	}

	t = malloc(sizeof(task_t));
	if (t == NULL) {
		rc = ENOMEM;
		goto finish;
	}

	/*
	 * Insert into the main table.
	 */
	task_init(t);
	t->id = id;

	hash_table_insert(&task_hash_table, &t->link);
	DPRINTF("%s: %llu\n", __func__, t->id);

finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
	return rc;
}

/**
 * @}
 */
