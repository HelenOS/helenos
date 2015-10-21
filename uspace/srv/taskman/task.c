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

/**
 * locking order: (TODO move to main?)
 * - task_hash_table_lock,
 * - pending_wait_lock.
 * - listeners_lock
 *
 * @addtogroup taskman
 * @{
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

static size_t task_key_hash(void *key)
{
	return *(task_id_t*)key;
}

static size_t task_hash(const ht_link_t  *item)
{
	task_t *ht = hash_table_get_inst(item, task_t, link);
	return ht->id;
}

static bool task_key_equal(void *key, const ht_link_t *item)
{
	task_t *ht = hash_table_get_inst(item, task_t, link);
	return ht->id == *(task_id_t*)key;
}

/** Perform actions after removal of item from the hash table. */
static void task_remove(ht_link_t *item)
{
	free(hash_table_get_inst(item, task_t, link));
}

/** Operations for task hash table. */
static hash_table_ops_t task_hash_table_ops = {
	.hash = task_hash,
	.key_hash = task_key_hash,
	.key_equal = task_key_equal,
	.equal = NULL,
	.remove_callback = task_remove
};

/** Task hash table structure. */
hash_table_t task_hash_table;
fibril_rwlock_t task_hash_table_lock;

int task_init(void)
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

int task_intro(ipc_call_t *call, bool check_unique)
{
	int rc = EOK;

	fibril_rwlock_write_lock(&task_hash_table_lock);

	task_t *t = task_get_by_id(call->in_task_id);
	if (t != NULL) {
		rc = EEXISTS;
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
	t->id = call->in_task_id;
	t->exit = TASK_EXIT_RUNNING;
	t->failed = false;
	t->retval_type = RVAL_UNSET;
	t->retval = -1;
	link_initialize(&t->listeners);
	t->sess = NULL;

	hash_table_insert(&task_hash_table, &t->link);
	printf("%s: %llu\n", __func__, t->id);
	
finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
	return rc;
}


/**
 * @}
 */
