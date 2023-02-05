/*
 * Copyright (c) 2009 Martin Decky
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup ns
 * @{
 */

#include <adt/hash_table.h>
#include <async.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <types/task.h>
#include "task.h"
#include "ns.h"

/** Task hash table item. */
typedef struct {
	ht_link_t link;

	task_id_t id;    /**< Task ID. */
	bool finished;   /**< Task is done. */
	bool have_rval;  /**< Task returned a value. */
	int retval;      /**< The return value. */
} hashed_task_t;

static size_t task_key_hash(const void *key)
{
	const task_id_t *tid = key;
	return *tid;
}

static size_t task_hash(const ht_link_t *item)
{
	hashed_task_t *ht = hash_table_get_inst(item, hashed_task_t, link);
	return ht->id;
}

static bool task_key_equal(const void *key, const ht_link_t *item)
{
	const task_id_t *tid = key;
	hashed_task_t *ht = hash_table_get_inst(item, hashed_task_t, link);
	return ht->id == *tid;
}

/** Perform actions after removal of item from the hash table. */
static void task_remove(ht_link_t *item)
{
	free(hash_table_get_inst(item, hashed_task_t, link));
}

/** Operations for task hash table. */
static const hash_table_ops_t task_hash_table_ops = {
	.hash = task_hash,
	.key_hash = task_key_hash,
	.key_equal = task_key_equal,
	.equal = NULL,
	.remove_callback = task_remove
};

/** Task hash table structure. */
static hash_table_t task_hash_table;

typedef struct {
	ht_link_t link;
	sysarg_t label;  /**< Incoming phone label. */
	task_id_t id;    /**< Task ID. */
} p2i_entry_t;

/* label-to-id hash table operations */

static size_t p2i_key_hash(const void *key)
{
	const sysarg_t *label = key;
	return *label;
}

static size_t p2i_hash(const ht_link_t *item)
{
	p2i_entry_t *entry = hash_table_get_inst(item, p2i_entry_t, link);
	return entry->label;
}

static bool p2i_key_equal(const void *key, const ht_link_t *item)
{
	const sysarg_t *label = key;
	p2i_entry_t *entry = hash_table_get_inst(item, p2i_entry_t, link);

	return (*label == entry->label);
}

/** Perform actions after removal of item from the hash table.
 *
 * @param item Item that was removed from the hash table.
 *
 */
static void p2i_remove(ht_link_t *item)
{
	assert(item);
	free(hash_table_get_inst(item, p2i_entry_t, link));
}

/** Operations for task hash table. */
static const hash_table_ops_t p2i_ops = {
	.hash = p2i_hash,
	.key_hash = p2i_key_hash,
	.key_equal = p2i_key_equal,
	.equal = NULL,
	.remove_callback = p2i_remove
};

/** Map phone label to task ID */
static hash_table_t phone_to_id;

/** Pending task wait structure. */
typedef struct {
	link_t link;
	task_id_t id;     /**< Task ID */
	ipc_call_t call;  /**< Call waiting for the connection */
} pending_wait_t;

static list_t pending_wait;

errno_t task_init(void)
{
	if (!hash_table_create(&task_hash_table, 0, 0, &task_hash_table_ops)) {
		printf(NAME ": No memory available for tasks\n");
		return ENOMEM;
	}

	if (!hash_table_create(&phone_to_id, 0, 0, &p2i_ops)) {
		printf(NAME ": No memory available for tasks\n");
		return ENOMEM;
	}

	list_initialize(&pending_wait);
	return EOK;
}

/** Process pending wait requests */
void process_pending_wait(void)
{
	task_exit_t texit;

loop:
	list_foreach(pending_wait, link, pending_wait_t, pr) {
		ht_link_t *link = hash_table_find(&task_hash_table, &pr->id);
		if (!link)
			continue;

		hashed_task_t *ht = hash_table_get_inst(link, hashed_task_t, link);
		if (!ht->finished)
			continue;

		texit = ht->have_rval ? TASK_EXIT_NORMAL :
		    TASK_EXIT_UNEXPECTED;
		async_answer_2(&pr->call, EOK, texit, ht->retval);

		list_remove(&pr->link);
		free(pr);
		goto loop;
	}
}

void wait_for_task(task_id_t id, ipc_call_t *call)
{
	ht_link_t *link = hash_table_find(&task_hash_table, &id);
	hashed_task_t *ht = (link != NULL) ?
	    hash_table_get_inst(link, hashed_task_t, link) : NULL;

	if (ht == NULL) {
		/* No such task exists. */
		async_answer_0(call, ENOENT);
		return;
	}

	if (ht->finished) {
		task_exit_t texit = ht->have_rval ? TASK_EXIT_NORMAL :
		    TASK_EXIT_UNEXPECTED;
		async_answer_2(call, EOK, texit, ht->retval);
		return;
	}

	/* Add to pending list */
	pending_wait_t *pr =
	    (pending_wait_t *) malloc(sizeof(pending_wait_t));
	if (!pr) {
		async_answer_0(call, ENOMEM);
		return;
	}

	link_initialize(&pr->link);
	pr->id = id;
	pr->call = *call;
	list_append(&pr->link, &pending_wait);
}

errno_t ns_task_id_intro(ipc_call_t *call)
{
	task_id_t id = MERGE_LOUP32(ipc_get_arg1(call), ipc_get_arg2(call));

	ht_link_t *link = hash_table_find(&phone_to_id, &call->request_label);
	if (link != NULL)
		return EEXIST;

	p2i_entry_t *entry = (p2i_entry_t *) malloc(sizeof(p2i_entry_t));
	if (entry == NULL)
		return ENOMEM;

	hashed_task_t *ht = (hashed_task_t *) malloc(sizeof(hashed_task_t));
	if (ht == NULL) {
		free(entry);
		return ENOMEM;
	}

	/*
	 * Insert into the phone-to-id map.
	 */

	assert(call->request_label);
	entry->label = call->request_label;
	entry->id = id;
	hash_table_insert(&phone_to_id, &entry->link);

	/*
	 * Insert into the main table.
	 */

	ht->id = id;
	ht->finished = false;
	ht->have_rval = false;
	ht->retval = -1;
	hash_table_insert(&task_hash_table, &ht->link);

	return EOK;
}

static errno_t get_id_by_phone(sysarg_t label, task_id_t *id)
{
	assert(label);
	ht_link_t *link = hash_table_find(&phone_to_id, &label);
	if (link == NULL)
		return ENOENT;

	p2i_entry_t *entry = hash_table_get_inst(link, p2i_entry_t, link);
	*id = entry->id;

	return EOK;
}

errno_t ns_task_retval(ipc_call_t *call)
{
	task_id_t id = call->task_id;

	ht_link_t *link = hash_table_find(&task_hash_table, &id);
	hashed_task_t *ht = (link != NULL) ?
	    hash_table_get_inst(link, hashed_task_t, link) : NULL;

	if ((ht == NULL) || (ht->finished))
		return EINVAL;

	ht->finished = true;
	ht->have_rval = true;
	ht->retval = ipc_get_arg1(call);

	process_pending_wait();

	return EOK;
}

errno_t ns_task_disconnect(ipc_call_t *call)
{
	task_id_t id;
	errno_t rc = get_id_by_phone(call->request_label, &id);
	if (rc != EOK)
		return rc;

	/* Delete from phone-to-id map. */
	hash_table_remove(&phone_to_id, &call->request_label);

	/* Mark task as finished. */
	ht_link_t *link = hash_table_find(&task_hash_table, &id);
	if (link == NULL)
		return EOK;

	hashed_task_t *ht = hash_table_get_inst(link, hashed_task_t, link);

	ht->finished = true;

	process_pending_wait();
	hash_table_remove(&task_hash_table, &id);

	return EOK;
}

/**
 * @}
 */
