/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#include <synch/synch.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <arch.h>
#include <errno.h>
#include <debug.h>
#include <udebug/udebug_ipc.h>
#include <ipc/kbox.h>

void ipc_kbox_cleanup(void)
{
	ipl_t ipl;
	bool have_kb_thread;

	/* 
	 * Only hold kb.cleanup_lock while setting kb.finished -
	 * this is enough.
	 */
	mutex_lock(&TASK->kb.cleanup_lock);
	TASK->kb.finished = true;
	mutex_unlock(&TASK->kb.cleanup_lock);

	have_kb_thread = (TASK->kb.thread != NULL);

	/*
	 * From now on nobody will try to connect phones or attach
	 * kbox threads
	 */

	/*
	 * Disconnect all phones connected to our kbox. Passing true for
	 * notify_box causes a HANGUP message to be inserted for each
	 * disconnected phone. This ensures the kbox thread is going to
	 * wake up and terminate.
	 */
	ipc_answerbox_slam_phones(&TASK->kb.box, have_kb_thread);

	/* 
	 * If the task was being debugged, clean up debugging session.
	 * This is necessarry as slamming the phones won't force
	 * kbox thread to clean it up since sender != debugger.
	 */
	ipl = interrupts_disable();
	spinlock_lock(&TASK->lock);
	udebug_task_cleanup(TASK);
	spinlock_unlock(&TASK->lock);
	interrupts_restore(ipl);
	
	if (have_kb_thread) {
		LOG("join kb.thread..\n");
		thread_join(TASK->kb.thread);
		thread_detach(TASK->kb.thread);
		LOG("join done\n");
		TASK->kb.thread = NULL;
	}

	/* Answer all messages in 'calls' and 'dispatched_calls' queues. */
	spinlock_lock(&TASK->kb.box.lock);
	ipc_cleanup_call_list(&TASK->kb.box.dispatched_calls);
	ipc_cleanup_call_list(&TASK->kb.box.calls);
	spinlock_unlock(&TASK->kb.box.lock);
}

/** Handle hangup message in kbox.
 *
 * @param call	The IPC_M_PHONE_HUNGUP call structure.
 * @param last	Output, the function stores @c true here if
 *		this was the last phone, @c false otherwise.
 **/
static void kbox_proc_phone_hungup(call_t *call, bool *last)
{
	ipl_t ipl;

	LOG("kbox_proc_phone_hungup()\n");

	/* Was it our debugger, who hung up? */
	if (call->sender == TASK->udebug.debugger) {
		/* Terminate debugging session (if any). */
		LOG("kbox: terminate debug session\n");
		ipl = interrupts_disable();
		spinlock_lock(&TASK->lock);
		udebug_task_cleanup(TASK);
		spinlock_unlock(&TASK->lock);
		interrupts_restore(ipl);
	} else {
		LOG("kbox: was not debugger\n");
	}

	LOG("kbox: continue with hangup message\n");
	IPC_SET_RETVAL(call->data, 0);
	ipc_answer(&TASK->kb.box, call);

	ipl = interrupts_disable();
	spinlock_lock(&TASK->lock);
	spinlock_lock(&TASK->kb.box.lock);
	if (list_empty(&TASK->kb.box.connected_phones)) {
		/*
		 * Last phone has been disconnected. Detach this thread so it
		 * gets freed and signal to the caller.
		 */

		/* Only detach kbox thread unless already terminating. */
		mutex_lock(&TASK->kb.cleanup_lock);
		if (&TASK->kb.finished == false) {
			/* Detach kbox thread so it gets freed from memory. */
			thread_detach(TASK->kb.thread);
			TASK->kb.thread = NULL;
		}
		mutex_unlock(&TASK->kb.cleanup_lock);

		LOG("phone list is empty\n");
		*last = true;
	} else {
		*last = false;
	}

	spinlock_unlock(&TASK->kb.box.lock);
	spinlock_unlock(&TASK->lock);
	interrupts_restore(ipl);
}

/** Implementing function for the kbox thread.
 *
 * This function listens for debug requests. It terminates
 * when all phones are disconnected from the kbox.
 *
 * @param arg	Ignored.
 */
static void kbox_thread_proc(void *arg)
{
	call_t *call;
	bool done;

	(void)arg;
	LOG("kbox_thread_proc()\n");
	done = false;

	while (!done) {
		call = ipc_wait_for_call(&TASK->kb.box, SYNCH_NO_TIMEOUT,
			SYNCH_FLAGS_NONE);

		if (call == NULL)
			continue;	/* Try again. */

		switch (IPC_GET_METHOD(call->data)) {

		case IPC_M_DEBUG_ALL:
			/* Handle debug call. */
			udebug_call_receive(call);
			break;

		case IPC_M_PHONE_HUNGUP:
			/*
			 * Process the hangup call. If this was the last
			 * phone, done will be set to true and the
			 * while loop will terminate.
			 */
			kbox_proc_phone_hungup(call, &done);
			break;

		default:
			/* Ignore */
			break;
		}
	}

	LOG("kbox: finished\n");
}


/**
 * Connect phone to a task kernel-box specified by id.
 *
 * Note that this is not completely atomic. For optimisation reasons, the task
 * might start cleaning up kbox after the phone has been connected and before
 * a kbox thread has been created. This must be taken into account in the
 * cleanup code.
 *
 * @return 		Phone id on success, or negative error code.
 */
int ipc_connect_kbox(task_id_t taskid)
{
	int newphid;
	task_t *ta;
	thread_t *kb_thread;
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

	ta = task_find_by_id(taskid);
	if (ta == NULL) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return ENOENT;
	}

	atomic_inc(&ta->refcount);

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);

	mutex_lock(&ta->kb.cleanup_lock);

	if (atomic_predec(&ta->refcount) == 0) {
		mutex_unlock(&ta->kb.cleanup_lock);
		task_destroy(ta);
		return ENOENT;
	}

	if (ta->kb.finished != false) {
		mutex_unlock(&ta->kb.cleanup_lock);
		return EINVAL;
	}

	newphid = phone_alloc();
	if (newphid < 0) {
		mutex_unlock(&ta->kb.cleanup_lock);
		return ELIMIT;
	}

	/* Connect the newly allocated phone to the kbox */
	ipc_phone_connect(&TASK->phones[newphid], &ta->kb.box);

	if (ta->kb.thread != NULL) {
		mutex_unlock(&ta->kb.cleanup_lock);
		return newphid;
	}

	/* Create a kbox thread */
	kb_thread = thread_create(kbox_thread_proc, NULL, ta, 0,
	    "kbox", false);
	if (!kb_thread) {
		mutex_unlock(&ta->kb.cleanup_lock);
		return ENOMEM;
	}

	ta->kb.thread = kb_thread;
	thread_ready(kb_thread);

	mutex_unlock(&ta->kb.cleanup_lock);

	return newphid;
}

/** @}
 */
