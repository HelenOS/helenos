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

#include <ipc/ipc.h>
#include <adt/hash_table.h>
#include <bool.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <macros.h>
#include "task.h"
#include "ns.h"

#define TASK_HASH_TABLE_CHAINS  256
#define P2I_HASH_TABLE_CHAINS  256

static int get_id_by_phone(ipcarg_t phone_hash, task_id_t *id);

/* TODO:
 *
 * As there is currently no convention that each task has to be waited
 * for, the NS can leak memory because of the zombie tasks.
 *
 */

/** Task hash table item. */
typedef struct {
	link_t link;
	task_id_t id;    /**< Task ID. */
	int retval;
	bool destroyed;
} hashed_task_t;

/** Compute hash index into task hash table.
 *
 * @param key Pointer keys. However, only the first key (i.e. truncated task
 *            number) is used to compute the hash index.
 *
 * @return Hash index corresponding to key[0].
 *
 */
static hash_index_t task_hash(unsigned long *key)
{
	assert(key);
	return (LOWER32(*key) % TASK_HASH_TABLE_CHAINS);
}

/** Compare a key with hashed item.
 *
 * @param key  Array of keys.
 * @param keys Must be less than or equal to 2.
 * @param item Pointer to a hash table item.
 *
 * @return Non-zero if the key matches the item, zero otherwise.
 *
 */
static int task_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(key);
	assert(keys <= 2);
	assert(item);
	
	hashed_task_t *ht = hash_table_get_instance(item, hashed_task_t, link);
	
	if (keys == 2)
		return ((LOWER32(key[1]) == UPPER32(ht->id))
		    && (LOWER32(key[0]) == LOWER32(ht->id)));
	else
		return (LOWER32(key[0]) == LOWER32(ht->id));
}

/** Perform actions after removal of item from the hash table.
 *
 * @param item Item that was removed from the hash table.
 *
 */
static void task_remove(link_t *item)
{
	assert(item);
	free(hash_table_get_instance(item, hashed_task_t, link));
}

/** Operations for task hash table. */
static hash_table_operations_t task_hash_table_ops = {
	.hash = task_hash,
	.compare = task_compare,
	.remove_callback = task_remove
};

/** Task hash table structure. */
static hash_table_t task_hash_table;

typedef struct {
	link_t link;
	ipcarg_t phash;    /**< Task ID. */
	task_id_t id;    /**< Task ID. */
} p2i_entry_t;

/** Compute hash index into task hash table.
 *
 * @param key Array of keys.
 * @return Hash index corresponding to key[0].
 *
 */
static hash_index_t p2i_hash(unsigned long *key)
{
	assert(key);
	return (*key % TASK_HASH_TABLE_CHAINS);
}

/** Compare a key with hashed item.
 *
 * @param key  Array of keys.
 * @param keys Must be less than or equal to 1.
 * @param item Pointer to a hash table item.
 *
 * @return Non-zero if the key matches the item, zero otherwise.
 *
 */
static int p2i_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(key);
	assert(keys == 1);
	assert(item);

	p2i_entry_t *e = hash_table_get_instance(item, p2i_entry_t, link);

	return (key[0] == e->phash);
}

/** Perform actions after removal of item from the hash table.
 *
 * @param item Item that was removed from the hash table.
 *
 */
static void p2i_remove(link_t *item)
{
	assert(item);
	free(hash_table_get_instance(item, p2i_entry_t, link));
}

/** Operations for task hash table. */
static hash_table_operations_t p2i_ops = {
	.hash = p2i_hash,
	.compare = p2i_compare,
	.remove_callback = p2i_remove
};

/** Map phone hash to task ID */
static hash_table_t phone_to_id;

/** Pending task wait structure. */
typedef struct {
	link_t link;
	task_id_t id;         /**< Task ID. */
	ipc_callid_t callid;  /**< Call ID waiting for the connection */
} pending_wait_t;

static link_t pending_wait;

int task_init(void)
{
	if (!hash_table_create(&task_hash_table, TASK_HASH_TABLE_CHAINS,
	    2, &task_hash_table_ops)) {
		printf(NAME ": No memory available for tasks\n");
		return ENOMEM;
	}

	if (!hash_table_create(&phone_to_id, P2I_HASH_TABLE_CHAINS,
	    1, &p2i_ops)) {
		printf(NAME ": No memory available for tasks\n");
		return ENOMEM;
	}
	
	list_initialize(&pending_wait);
	
	return EOK;
}

/** Process pending wait requests */
void process_pending_wait(void)
{
	link_t *cur;
	
loop:
	for (cur = pending_wait.next; cur != &pending_wait; cur = cur->next) {
		pending_wait_t *pr = list_get_instance(cur, pending_wait_t, link);
		
		unsigned long keys[2] = {
			LOWER32(pr->id),
			UPPER32(pr->id)
		};
		
		link_t *link = hash_table_find(&task_hash_table, keys);
		if (!link)
			continue;
		
		hashed_task_t *ht = hash_table_get_instance(link, hashed_task_t, link);
		if (!ht->destroyed)
			continue;
		
		if (!(pr->callid & IPC_CALLID_NOTIFICATION))
			ipc_answer_1(pr->callid, EOK, ht->retval);
		
		hash_table_remove(&task_hash_table, keys, 2);
		list_remove(cur);
		free(pr);
		goto loop;
	}
}

void wait_for_task(task_id_t id, ipc_call_t *call, ipc_callid_t callid)
{
	ipcarg_t retval;
	unsigned long keys[2] = {
		LOWER32(id),
		UPPER32(id)
	};

	link_t *link = hash_table_find(&task_hash_table, keys);
	hashed_task_t *ht = (link != NULL) ?
	    hash_table_get_instance(link, hashed_task_t, link) : NULL;

	if (ht == NULL) {
		/* No such task exists. */
		retval = ENOENT;
		goto out;
	}

	if (!ht->destroyed) {
		/* Add to pending list */
		pending_wait_t *pr =
		    (pending_wait_t *) malloc(sizeof(pending_wait_t));
		if (!pr) {
			retval = ENOMEM;
			goto out;
		}
		
		pr->id = id;
		pr->callid = callid;
		list_append(&pr->link, &pending_wait);
		return;
	}
	
	hash_table_remove(&task_hash_table, keys, 2);
	retval = EOK;
	
out:
	if (!(callid & IPC_CALLID_NOTIFICATION))
		ipc_answer_1(callid, retval, ht->retval);
}

int ns_task_id_intro(ipc_call_t *call)
{
	task_id_t id;
	unsigned long keys[2];
	link_t *link;
	p2i_entry_t *e;
	hashed_task_t *ht;

	id = MERGE_LOUP32(IPC_GET_ARG1(*call), IPC_GET_ARG2(*call));

	keys[0] = call->in_phone_hash;

	link = hash_table_find(&phone_to_id, keys);
	if (link != NULL)
		return EEXISTS;

	e = (p2i_entry_t *) malloc(sizeof(p2i_entry_t));
	if (e == NULL)
		return ENOMEM;

	ht = (hashed_task_t *) malloc(sizeof(hashed_task_t));
	if (ht == NULL)
		return ENOMEM;

	/* Insert to phone-to-id map. */

	link_initialize(&e->link);
	e->phash = call->in_phone_hash;
	e->id = id;
	hash_table_insert(&phone_to_id, keys, &e->link);

	/* Insert to main table. */

	keys[0] = LOWER32(id);
	keys[1] = UPPER32(id);

	link_initialize(&ht->link);
	ht->id = id;
	ht->destroyed = false;
	ht->retval = -1;
	hash_table_insert(&task_hash_table, keys, &ht->link);

	return EOK;
}

int ns_task_retval(ipc_call_t *call)
{
	task_id_t id;
	unsigned long keys[2];
	int rc;

	rc = get_id_by_phone(call->in_phone_hash, &id);
	if (rc != EOK)
		return rc;

	keys[0] = LOWER32(id);
	keys[1] = UPPER32(id);
	
	link_t *link = hash_table_find(&task_hash_table, keys);
	hashed_task_t *ht = (link != NULL) ?
	    hash_table_get_instance(link, hashed_task_t, link) : NULL;
	
	if ((ht == NULL) || ht->destroyed)
		return EINVAL;

	ht->retval = IPC_GET_ARG1(*call);

	return EOK;
}

int ns_task_disconnect(ipc_call_t *call)
{
	unsigned long keys[2];
	task_id_t id;
	int rc;

	rc = get_id_by_phone(call->in_phone_hash, &id);
	if (rc != EOK)
		return rc;

	/* Delete from phone-to-id map. */
	keys[0] = call->in_phone_hash;
	hash_table_remove(&phone_to_id, keys, 1);

	/* Mark task as finished. */
	keys[0] = LOWER32(id);
	keys[1] = UPPER32(id);

	link_t *link = hash_table_find(&task_hash_table, keys);
	hashed_task_t *ht =
	    hash_table_get_instance(link, hashed_task_t, link);
	assert(ht != NULL);

	ht->destroyed = true;

	return EOK;
}

static int get_id_by_phone(ipcarg_t phone_hash, task_id_t *id)
{
	unsigned long keys[1];
	link_t *link;
	p2i_entry_t *e;

	keys[0] = phone_hash;
	link = hash_table_find(&phone_to_id, keys);
	if (link == NULL)
		return ENOENT;

	e = hash_table_get_instance(link, p2i_entry_t, link);
	*id = e->id;

	return EOK;
}

/**
 * @}
 */
