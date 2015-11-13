/*
 * Copyright (c) 2015 Michal Koutny
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
 * Locking order:
 * - task_hash_table_lock (task.c),
 * - pending_wait_lock (event.c),
 * - listeners_lock (event.c).
 *
 * @addtogroup taskman
 * @{
 */

#include <adt/prodcons.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ipc/services.h>
#include <ipc/taskman.h>
#include <loader/loader.h>
#include <macros.h>
#include <ns.h>
#include <stdio.h>
#include <stdlib.h>

#include "event.h"
#include "task.h"
#include "taskman.h"


typedef struct {
	link_t link;
	async_sess_t *sess;
} sess_ref_t;

static prodcons_t sess_queue;

/** We keep session to NS on our own in taskman */
static async_sess_t *session_ns = NULL;

static FIBRIL_MUTEX_INITIALIZE(session_ns_mtx);
static FIBRIL_CONDVAR_INITIALIZE(session_ns_cv);

/*
 * Static functions
 */
static void connect_to_loader(ipc_callid_t iid, ipc_call_t *icall)
{
	DPRINTF("%s:%i from %llu\n", __func__, __LINE__, icall->in_task_id);
	/* We don't accept the connection request, we forward it instead to
	 * freshly spawned loader. */
	int rc = loader_spawn("loader");
	
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	/* Wait until spawned task presents itself to us. */
	link_t *link = prodcons_consume(&sess_queue);
	sess_ref_t *sess_ref = list_get_instance(link, sess_ref_t, link);

	/* Forward the connection request (strip interface arg). */
	async_exch_t *exch = async_exchange_begin(sess_ref->sess);
	rc = async_forward_fast(iid, exch,
	    IPC_GET_ARG2(*icall),
	    IPC_GET_ARG3(*icall),
	    0, IPC_FF_NONE);
	async_exchange_end(exch);

	/* After forward we can dispose all session-related resources */
	async_hangup(sess_ref->sess);
	free(sess_ref);

	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	/* Everything OK. */
}

static void connect_to_ns(ipc_callid_t iid, ipc_call_t *icall)
{
	DPRINTF("%s, %llu\n", __func__, icall->in_task_id);

	/* Wait until we know NS */
	fibril_mutex_lock(&session_ns_mtx);
	while (session_ns == NULL) {
		fibril_condvar_wait(&session_ns_cv, &session_ns_mtx);
	}
	fibril_mutex_unlock(&session_ns_mtx);

	/* Do not accept connection, forward it */
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = async_forward_fast(iid, exch, 0, 0, 0, IPC_FF_NONE);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
}

static void taskman_new_task(ipc_callid_t iid, ipc_call_t *icall)
{
	int rc = task_intro(icall->in_task_id);
	async_answer_0(iid, rc);
}

static void taskman_i_am_ns(ipc_callid_t iid, ipc_call_t *icall)
{
	DPRINTF("%s, %llu\n", __func__, icall->in_task_id);
	int rc = EOK;

	fibril_mutex_lock(&session_ns_mtx);
	if (session_ns != NULL) {
		rc = EEXISTS;
		goto finish;
	}

	/* Used only for connection forwarding -- atomic */
	session_ns = async_callback_receive(EXCHANGE_ATOMIC);
	
	if (session_ns == NULL) {
		rc = ENOENT;
		printf("%s: Cannot connect to NS\n", NAME);
	}

	fibril_condvar_broadcast(&session_ns_cv);
finish:
	fibril_mutex_unlock(&session_ns_mtx);
	async_answer_0(iid, rc);
}

static void taskman_ctl_wait(ipc_callid_t iid, ipc_call_t *icall)
{
	task_id_t id = (task_id_t)
	    MERGE_LOUP32(IPC_GET_ARG1(*icall), IPC_GET_ARG2(*icall));
	int flags = IPC_GET_ARG3(*icall);
	task_id_t waiter_id = icall->in_task_id;

	wait_for_task(id, flags, iid, waiter_id);
}

static void taskman_ctl_retval(ipc_callid_t iid, ipc_call_t *icall)
{
	task_id_t sender = icall->in_task_id;
	int retval = IPC_GET_ARG1(*icall);
	bool wait_for_exit = IPC_GET_ARG2(*icall);

	DPRINTF("%s:%i from %llu/%i\n", __func__, __LINE__, sender, retval);

	int rc = task_set_retval(sender, retval, wait_for_exit);
	async_answer_0(iid, rc);
}

static void taskman_ctl_ev_callback(ipc_callid_t iid, ipc_call_t *icall)
{
	DPRINTF("%s:%i from %llu\n", __func__, __LINE__, icall->in_task_id);

	/* Atomic -- will be used for notifications only */
	async_sess_t *sess = async_callback_receive(EXCHANGE_ATOMIC);
	if (sess == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	int rc = event_register_listener(icall->in_task_id, sess);
	async_answer_0(iid, rc);
}

static void taskman_ctl_dump_events(ipc_callid_t iid, ipc_call_t *icall)
{
	DPRINTF("%s:%i from %llu\n", __func__, __LINE__, icall->in_task_id);

	dump_events(icall->in_task_id, iid);
}

static void task_exit_event(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	task_id_t id = MERGE_LOUP32(IPC_GET_ARG1(*icall), IPC_GET_ARG2(*icall));
	exit_reason_t exit_reason = IPC_GET_ARG3(*icall);
	DPRINTF("%s:%i from %llu/%i\n", __func__, __LINE__, id, exit_reason);
	task_terminated(id, exit_reason);
}

static void task_fault_event(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	task_id_t id = MERGE_LOUP32(IPC_GET_ARG1(*icall), IPC_GET_ARG2(*icall));
	DPRINTF("%s:%i from %llu\n", __func__, __LINE__, id);
	task_failed(id);
}

static void loader_callback(ipc_callid_t iid, ipc_call_t *icall)
{
	DPRINTF("%s:%i from %llu\n", __func__, __LINE__, icall->in_task_id);
	// TODO check that loader is expected, would probably discard prodcons
	//      scheme
	
	/* Preallocate session container */
	sess_ref_t *sess_ref = malloc(sizeof(sess_ref_t));
	if (sess_ref == NULL) {
		async_answer_0(iid, ENOMEM);
	}

	/* Create callback connection */
	sess_ref->sess = async_callback_receive_start(EXCHANGE_ATOMIC, icall);
	if (sess_ref->sess == NULL) {
		async_answer_0(iid, EINVAL);
		return;
	}

	async_answer_0(iid, EOK);

	/* Notify spawners */
	link_initialize(&sess_ref->link);
	prodcons_produce(&sess_queue, &sess_ref->link);
}

static bool handle_call(ipc_callid_t iid, ipc_call_t *icall)
{
	switch (IPC_GET_IMETHOD(*icall)) {
	case TASKMAN_NEW_TASK:
		taskman_new_task(iid, icall);
		break;
	case TASKMAN_I_AM_NS:
		taskman_i_am_ns(iid, icall);
		break;
	case TASKMAN_WAIT:
		taskman_ctl_wait(iid, icall);
		break;
	case TASKMAN_RETVAL:
		taskman_ctl_retval(iid, icall);
		break;
	case TASKMAN_EVENT_CALLBACK:
		taskman_ctl_ev_callback(iid, icall);
		break;
	case TASKMAN_DUMP_EVENTS:
		taskman_ctl_dump_events(iid, icall);
		break;
	default:
		return false;
	}
	return true;
}

static bool handle_implicit_call(ipc_callid_t iid, ipc_call_t *icall)
{
	DPRINTF("%s:%i %i(%i) from %llu\n", __func__, __LINE__,
	    IPC_GET_IMETHOD(*icall),
	    IPC_GET_ARG1(*icall),
	    icall->in_task_id);

	if (IPC_GET_IMETHOD(*icall) < IPC_FIRST_USER_METHOD) {
		switch (IPC_GET_ARG1(*icall)) {
		case TASKMAN_CONNECT_TO_NS:
			connect_to_ns(iid, icall);
			break;
		case TASKMAN_CONNECT_TO_LOADER:
			connect_to_loader(iid, icall);
			break;
		case TASKMAN_LOADER_CALLBACK:
			loader_callback(iid, icall);
			break;
		default:
			return false;

		}
	} else {
		return handle_call(iid, icall);
	}

	return true;
}

static void implicit_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	if (!handle_implicit_call(iid, icall)) {
		async_answer_0(iid, ENOTSUP);
		return;
	}

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* Client disconnected */
			break;
		}

		if (!handle_implicit_call(callid, &call)) {
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
}

static void taskman_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/*
	 * We don't expect (yet) clients to connect, having this function is
	 * just to adapt to async framework that creates new connection for
	 * each IPC_M_CONNECT_ME_TO.
	 * In this case those are to be forwarded, so don't continue
	 * "listening" on such connections.
	 */
	if (!handle_implicit_call(iid, icall)) {
		/* If cannot handle connection requst, give up trying */
		async_answer_0(iid, EHANGUP);
		return;
	}
}



int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS task manager\n");

	/* Initialization */
	prodcons_initialize(&sess_queue);
	int rc = tasks_init();
	if (rc != EOK) {
		return rc;
	}
	rc = event_init();
	if (rc != EOK) {
		return rc;
	}

	rc = async_event_subscribe(EVENT_EXIT, task_exit_event, NULL);
	if (rc != EOK) {
		printf(NAME ": Cannot register for exit events (%i).\n", rc);
		return rc;
	}

	rc = async_event_subscribe(EVENT_FAULT, task_fault_event, NULL);
	if (rc != EOK) {
		printf(NAME ": Cannot register for fault events (%i).\n", rc);
		return rc;
	}
	
	task_id_t self_id = task_get_id();
	rc = task_intro(self_id);
	if (rc != EOK) {
		printf(NAME ": Cannot register self as task (%i).\n", rc);
	}

	/* Start sysman server */
	async_set_implicit_connection(implicit_connection);
	async_set_client_connection(taskman_connection);

	printf(NAME ": Accepting connections\n");
	(void)task_set_retval(self_id, EOK, false);
	async_manager();

	/* not reached */
	return 0;
}

/**
 * @}
 */
