/*
 * copyright (c) 2009 martin decky
 * copyright (c) 2009 jiri svoboda
 * copyright (c) 2015 michal koutny
 * all rights reserved.
 *
 * redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - the name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * this software is provided by the author ``as is'' and any express or
 * implied warranties, including, but not limited to, the implied warranties
 * of merchantability and fitness for a particular purpose are disclaimed.
 * in no event shall the author be liable for any direct, indirect,
 * incidental, special, exemplary, or consequential damages (including, but
 * not limited to, procurement of substitute goods or services; loss of use,
 * data, or profits; or business interruption) however caused and on any
 * theory of liability, whether in contract, strict liability, or tort
 * (including negligence or otherwise) arising in any way out of the use of
 * this software, even if advised of the possibility of such damage.
 */

/**
 * locking order:
 * - task_hash_table_lock,
 * - pending_wait_lock.
 *
 * @addtogroup taskman
 * @{
 */

#include <adt/hash_table.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <macros.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <task.h>
#include <types/task.h>

#include "task.h"
#include "taskman.h"

/** what type of retval from the task we have */
typedef enum {
	RVAL_UNSET,     /**< unset */
	RVAL_SET,       /**< retval set, e.g. by server */
	RVAL_SET_EXIT   /**< retval set, wait for expected task exit */
} retval_t;

/** task hash table item. */
typedef struct {
	ht_link_t link;
	
	task_id_t id;        /**< task id. */
	task_exit_t exit;    /**< task is done. */
	retval_t retval_type;  /**< task returned a value. */
	int retval;          /**< the return value. */
} hashed_task_t;


static size_t task_key_hash(void *key)
{
	return *(task_id_t*)key;
}

static size_t task_hash(const ht_link_t  *item)
{
	hashed_task_t *ht = hash_table_get_inst(item, hashed_task_t, link);
	return ht->id;
}

static bool task_key_equal(void *key, const ht_link_t *item)
{
	hashed_task_t *ht = hash_table_get_inst(item, hashed_task_t, link);
	return ht->id == *(task_id_t*)key;
}

/** perform actions after removal of item from the hash table. */
static void task_remove(ht_link_t *item)
{
	free(hash_table_get_inst(item, hashed_task_t, link));
}

/** operations for task hash table. */
static hash_table_ops_t task_hash_table_ops = {
	.hash = task_hash,
	.key_hash = task_key_hash,
	.key_equal = task_key_equal,
	.equal = NULL,
	.remove_callback = task_remove
};

/** Task hash table structure. */
static hash_table_t task_hash_table;
static FIBRIL_RWLOCK_INITIALIZE(task_hash_table_lock);

/** Pending task wait structure. */
typedef struct {
	link_t link;
	task_id_t id;         /**< Task ID who we wait for. */
	task_id_t waiter_id;  /**< Task ID who waits. */
	ipc_callid_t callid;  /**< Call ID waiting for the connection */
	int flags;            /**< Wait flags */
} pending_wait_t;

static list_t pending_wait;
static FIBRIL_RWLOCK_INITIALIZE(pending_wait_lock);

int task_init(void)
{
	if (!hash_table_create(&task_hash_table, 0, 0, &task_hash_table_ops)) {
		printf(NAME ": No memory available for tasks\n");
		return ENOMEM;
	}
	
	list_initialize(&pending_wait);
	return EOK;
}

/** Process pending wait requests
 *
 * Assumes task_hash_table_lock is hold (at least read)
 */
void process_pending_wait(void)
{
	fibril_rwlock_write_lock(&pending_wait_lock);
loop:
	list_foreach(pending_wait, link, pending_wait_t, pr) {
		ht_link_t *link = hash_table_find(&task_hash_table, &pr->id);
		if (!link)
			continue;
		
		hashed_task_t *ht = hash_table_get_inst(link, hashed_task_t, link);
		int notify_flags = 0;
		if (ht->exit != TASK_EXIT_RUNNING) {
			notify_flags |= TASK_WAIT_EXIT;
			if (ht->retval_type == RVAL_SET_EXIT) {
				notify_flags |= TASK_WAIT_RETVAL;
			}
		}
		if (ht->retval_type == RVAL_SET) {
			notify_flags |= TASK_WAIT_RETVAL;
		}

		/*
		 * In current implementation you can wait for single retval,
		 * thus it can be never present in rest flags.
		 */
		int rest = (~notify_flags & pr->flags) & ~TASK_WAIT_RETVAL;
		rest &= ~TASK_WAIT_BOTH;
		int match = notify_flags & pr->flags;
		bool answer = !(pr->callid & IPC_CALLID_NOTIFICATION);
		printf("%s: %x; %x, %x\n", __func__, pr->flags, rest, match);

		if (match == 0) {
			if (notify_flags & TASK_WAIT_EXIT) {
				/* Nothing to wait for anymore */
				if (answer) {
					async_answer_0(pr->callid, EINVAL);
				}
			} else {
				/* Maybe later */
				continue;
			}
		} else if (answer) {
			if ((pr->flags & TASK_WAIT_BOTH) && match == TASK_WAIT_EXIT) {
				async_answer_1(pr->callid, EINVAL, ht->exit);
			} else {
				/* Send both exit status and retval, caller
				 * should know what is valid */
				async_answer_3(pr->callid, EOK, ht->exit,
				    ht->retval, rest);
			}

			/* Pending wait has one more chance  */
			if (rest && (pr->flags & TASK_WAIT_BOTH)) {
				pr->flags = rest | TASK_WAIT_BOTH;
				continue;
			}
		}

		
		list_remove(&pr->link);
		free(pr);
		goto loop;
	}
	fibril_rwlock_write_unlock(&pending_wait_lock);
}

void wait_for_task(task_id_t id, int flags, ipc_callid_t callid, ipc_call_t *call)
{
	assert(!(flags & TASK_WAIT_BOTH) ||
	    ((flags & TASK_WAIT_RETVAL) && (flags & TASK_WAIT_EXIT)));

	fibril_rwlock_read_lock(&task_hash_table_lock);
	ht_link_t *link = hash_table_find(&task_hash_table, &id);
	fibril_rwlock_read_unlock(&task_hash_table_lock);

	hashed_task_t *ht = (link != NULL) ?
	    hash_table_get_inst(link, hashed_task_t, link) : NULL;
	
	if (ht == NULL) {
		/* No such task exists. */
		async_answer_0(callid, ENOENT);
		return;
	}
	
	if (ht->exit != TASK_EXIT_RUNNING) {
		//TODO are flags BOTH processed correctly here?
		async_answer_3(callid, EOK, ht->exit, ht->retval, 0);
		return;
	}
	
	/*
	 * Add request to pending list or reuse existing item for a second
	 * wait.
	 */
	task_id_t waiter_id = call->in_task_id;
	fibril_rwlock_write_lock(&pending_wait_lock);
	pending_wait_t *pr = NULL;
	list_foreach(pending_wait, link, pending_wait_t, it) {
		if (it->id == id && it->waiter_id == waiter_id) {
			pr = it;
			break;
		}
	}

	int rc = EOK;
	bool reuse = false;
	if (pr == NULL) {
		pr = malloc(sizeof(pending_wait_t));
		if (!pr) {
			rc = ENOMEM;
			goto finish;
		}
	
		link_initialize(&pr->link);
		pr->id = id;
		pr->waiter_id = waiter_id;
		pr->flags = flags;
		pr->callid = callid;

		list_append(&pr->link, &pending_wait);
		rc = EOK;
	} else if (!(pr->flags & TASK_WAIT_BOTH)) {
		/*
		 * One task can wait for another task only once (per task, not
		 * fibril).
		 */
		rc = EEXISTS;
	} else {
		/*
		 * Reuse pending wait for the second time.
		 */
		pr->flags &= ~TASK_WAIT_BOTH; // TODO maybe new flags should be set?
		pr->callid = callid;
		reuse = true;
	}
	printf("%s: %llu: %x, %x, %i\n", __func__, pr->id, flags, pr->flags, reuse);

finish:
	fibril_rwlock_write_unlock(&pending_wait_lock);
	// TODO why IPC_CALLID_NOTIFICATION? explain!
	if (rc != EOK && !(callid & IPC_CALLID_NOTIFICATION))
		async_answer_0(callid, rc);

}

int task_intro(ipc_call_t *call, bool check_unique)
{
	int rc = EOK;

	fibril_rwlock_write_lock(&task_hash_table_lock);

	ht_link_t *link = hash_table_find(&task_hash_table, &call->in_task_id);
	if (link != NULL) {
		rc = EEXISTS;
		goto finish;
	}
	
	hashed_task_t *ht = (hashed_task_t *) malloc(sizeof(hashed_task_t));
	if (ht == NULL) {
		rc = ENOMEM;
		goto finish;
	}

	/*
	 * Insert into the main table.
	 */
	ht->id = call->in_task_id;
	ht->exit = TASK_EXIT_RUNNING;
	ht->retval_type = RVAL_UNSET;
	ht->retval = -1;

	hash_table_insert(&task_hash_table, &ht->link);
	printf("%s: %llu\n", __func__, ht->id);
	
finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
	return rc;
}

int task_set_retval(ipc_call_t *call)
{
	int rc = EOK;
	task_id_t id = call->in_task_id;
	
	fibril_rwlock_write_lock(&task_hash_table_lock);
	ht_link_t *link = hash_table_find(&task_hash_table, &id);

	hashed_task_t *ht = (link != NULL) ?
	    hash_table_get_inst(link, hashed_task_t, link) : NULL;
	
	if ((ht == NULL) || (ht->exit != TASK_EXIT_RUNNING)) {
		rc = EINVAL;
		goto finish;
	}
	
	ht->retval = IPC_GET_ARG1(*call);
	ht->retval_type = IPC_GET_ARG2(*call) ? RVAL_SET_EXIT : RVAL_SET;
	
	process_pending_wait();
	
finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
	return rc;
}

void task_terminated(task_id_t id, task_exit_t texit)
{
	/* Mark task as finished. */
	fibril_rwlock_write_lock(&task_hash_table_lock);
	ht_link_t *link = hash_table_find(&task_hash_table, &id);
	if (link == NULL) {
		goto finish;
	}

	hashed_task_t *ht = hash_table_get_inst(link, hashed_task_t, link);
	
	/*
	 * If daemon returns a value and then fails/is killed, it's unexpected
	 * termination.
	 */
	if (ht->retval_type == RVAL_UNSET || texit == TASK_EXIT_UNEXPECTED) {
		ht->exit = TASK_EXIT_UNEXPECTED;
	} else {
		ht->exit = texit;
	}
	process_pending_wait();

	hash_table_remove_item(&task_hash_table, &ht->link);
finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
}

/**
 * @}
 */
