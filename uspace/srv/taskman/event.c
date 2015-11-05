/*
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
 *
 * @addtogroup taskman
 * @{
 */

#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <ipc/taskman.h>
#include <stdlib.h>
#include <task.h>

#include "event.h"
#include "task.h"

/** Pending task wait structure. */
typedef struct {
	link_t link;
	task_id_t id;         /**< Task ID who we wait for. */
	task_id_t waiter_id;  /**< Task ID who waits. */
	ipc_callid_t callid;  /**< Call ID waiting for the event. */
	int flags;            /**< Wait flags. */
} pending_wait_t;

static list_t pending_waits;
static FIBRIL_RWLOCK_INITIALIZE(pending_wait_lock);

static list_t listeners;
static FIBRIL_RWLOCK_INITIALIZE(listeners_lock);

int event_init(void)
{
	list_initialize(&pending_waits);
	list_initialize(&listeners);

	return EOK;
}

static int event_flags(task_t *task)
{
	int flags = 0;
	if (task->exit != TASK_EXIT_RUNNING) {
		flags |= TASK_WAIT_EXIT;
		if (task->retval_type == RVAL_SET_EXIT) {
			flags |= TASK_WAIT_RETVAL;
		}
	}
	if (task->retval_type == RVAL_SET) {
		flags |= TASK_WAIT_RETVAL;
	}

	return flags;
}

static void event_notify(task_t *sender)
{
	// TODO should rlock task_hash_table?
	int flags = event_flags(sender);
	if (flags == 0) {
		return;
	}

	fibril_rwlock_read_lock(&listeners_lock);
	list_foreach(listeners, listeners, task_t, t) {
		assert(t->sess);
		async_exch_t *exch = async_exchange_begin(t->sess);
		aid_t req = async_send_5(exch, TASKMAN_EV_TASK,
		    LOWER32(sender->id),
		    UPPER32(sender->id),
		    flags,
		    sender->exit,
		    sender->retval,
		    NULL);

		async_exchange_end(exch);

		/* Just send a notification and don't wait for anything */
		async_forget(req);
	}
	fibril_rwlock_read_unlock(&listeners_lock);
}

/** Process pending wait requests
 *
 * Assumes task_hash_table_lock is hold (at least read)
 */
static void process_pending_wait(void)
{
	fibril_rwlock_write_lock(&pending_wait_lock);
loop:
	list_foreach(pending_waits, link, pending_wait_t, pr) {
		task_t *t = task_get_by_id(pr->id);
		if (t == NULL) {
			continue; // TODO really when does this happen?
		}
		int notify_flags = event_flags(t);

		/*
		 * In current implementation you can wait for single retval,
		 * thus it can be never present in rest flags.
		 */
		int rest = (~notify_flags & pr->flags) & ~TASK_WAIT_RETVAL;
		rest &= ~TASK_WAIT_BOTH;
		int match = notify_flags & pr->flags;
		// TODO why do I even accept such calls?
		bool answer = !(pr->callid & IPC_CALLID_NOTIFICATION);

		if (match == 0) {
			if (notify_flags & TASK_WAIT_EXIT) {
				/* Nothing to wait for anymore */
				if (answer) {
					async_answer_0(pr->callid, EINTR);
				}
			} else {
				/* Maybe later */
				continue;
			}
		} else if (answer) {
			if ((pr->flags & TASK_WAIT_BOTH) && match == TASK_WAIT_EXIT) {
				/* No sense to wait for both anymore */
				async_answer_1(pr->callid, EINTR, t->exit);
			} else {
				/* Send both exit status and retval, caller
				 * should know what is valid */
				async_answer_3(pr->callid, EOK, t->exit,
				    t->retval, rest);
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

int event_register_listener(task_id_t id, async_sess_t *sess)
{
	int rc = EOK;
	fibril_rwlock_write_lock(&task_hash_table_lock);
	fibril_rwlock_write_lock(&listeners_lock);

	task_t *t = task_get_by_id(id);
	if (t == NULL) {
		rc = ENOENT;
		goto finish;
	}
	assert(t->sess == NULL);
	list_append(&t->listeners, &listeners);
	t->sess = sess;

finish:
	fibril_rwlock_write_unlock(&listeners_lock);
	fibril_rwlock_write_unlock(&task_hash_table_lock);
	return rc;
}

void wait_for_task(task_id_t id, int flags, ipc_callid_t callid,
     task_id_t waiter_id)
{
	assert(!(flags & TASK_WAIT_BOTH) ||
	    ((flags & TASK_WAIT_RETVAL) && (flags & TASK_WAIT_EXIT)));

	fibril_rwlock_read_lock(&task_hash_table_lock);
	task_t *t = task_get_by_id(id);
	fibril_rwlock_read_unlock(&task_hash_table_lock);

	if (t == NULL) {
		/* No such task exists. */
		async_answer_0(callid, ENOENT);
		return;
	}
	
	if (t->exit != TASK_EXIT_RUNNING) {
		//TODO are flags BOTH processed correctly here?
		async_answer_3(callid, EOK, t->exit, t->retval, 0);
		return;
	}
	
	/*
	 * Add request to pending list or reuse existing item for a second
	 * wait.
	 */
	fibril_rwlock_write_lock(&pending_wait_lock);
	pending_wait_t *pr = NULL;
	list_foreach(pending_waits, link, pending_wait_t, it) {
		if (it->id == id && it->waiter_id == waiter_id) {
			pr = it;
			break;
		}
	}

	int rc = EOK;
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

		list_append(&pr->link, &pending_waits);
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
	}

finish:
	fibril_rwlock_write_unlock(&pending_wait_lock);
	// TODO why IPC_CALLID_NOTIFICATION? explain!
	if (rc != EOK && !(callid & IPC_CALLID_NOTIFICATION)) {
		async_answer_0(callid, rc);
	}
}


int task_set_retval(task_id_t sender, int retval, bool wait_for_exit)
{
	int rc = EOK;
	
	fibril_rwlock_write_lock(&task_hash_table_lock);
	task_t *t = task_get_by_id(sender);

	if ((t == NULL) || (t->exit != TASK_EXIT_RUNNING)) {
		rc = EINVAL;
		goto finish;
	}
	
	t->retval = retval;
	t->retval_type = wait_for_exit ? RVAL_SET_EXIT : RVAL_SET;
	
	event_notify(t);
	process_pending_wait();
	
finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
	return rc;
}

void task_terminated(task_id_t id, exit_reason_t exit_reason)
{
	/* Mark task as finished. */
	fibril_rwlock_write_lock(&task_hash_table_lock);
	task_t *t = task_get_by_id(id);
	if (t == NULL) {
		goto finish;
	}

	/*
	 * If daemon returns a value and then fails/is killed, it's an
	 * unexpected termination.
	 */
	if (t->retval_type == RVAL_UNSET || exit_reason == EXIT_REASON_KILLED) {
		t->exit = TASK_EXIT_UNEXPECTED;
	} else if (t->failed) {
		t->exit = TASK_EXIT_UNEXPECTED;
	} else  {
		t->exit = TASK_EXIT_NORMAL;
	}

	event_notify(t);
	process_pending_wait();

	hash_table_remove_item(&task_hash_table, &t->link);

	fibril_rwlock_write_lock(&listeners_lock);
	list_remove(&t->listeners);
	fibril_rwlock_write_unlock(&listeners_lock);

finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
}

void task_failed(task_id_t id)
{
	/* Mark task as failed. */
	fibril_rwlock_write_lock(&task_hash_table_lock);
	task_t *t = task_get_by_id(id);
	if (t == NULL) {
		goto finish;
	}

	t->failed = true;
	// TODO design substitution for taskmon (monitoring) = invoke dump
	// utility or pass event to registered tasks

finish:
	fibril_rwlock_write_unlock(&task_hash_table_lock);
}

/**
 * @}
 */
