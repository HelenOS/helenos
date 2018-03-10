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

#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <ipc/ipc.h>
#include <abi/ipc/methods.h>
#include <ipc/ipcrsc.h>
#include <arch.h>
#include <errno.h>
#include <debug.h>
#include <udebug/udebug_ipc.h>
#include <ipc/kbox.h>
#include <print.h>
#include <proc/thread.h>

void ipc_kbox_cleanup(void)
{
	/*
	 * Not really needed, just to be consistent with the meaning of
	 * answerbox_t.active.
	 */
	irq_spinlock_lock(&TASK->kb.box.lock, true);
	TASK->kb.box.active = false;
	irq_spinlock_unlock(&TASK->kb.box.lock, true);

	/*
	 * Only hold kb.cleanup_lock while setting kb.finished -
	 * this is enough.
	 */
	mutex_lock(&TASK->kb.cleanup_lock);
	TASK->kb.finished = true;
	mutex_unlock(&TASK->kb.cleanup_lock);

	bool have_kb_thread = (TASK->kb.thread != NULL);

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
	mutex_lock(&TASK->udebug.lock);
	udebug_task_cleanup(TASK);
	mutex_unlock(&TASK->udebug.lock);

	if (have_kb_thread) {
		LOG("Join kb.thread.");
		thread_join(TASK->kb.thread);
		thread_detach(TASK->kb.thread);
		LOG("...join done.");
		TASK->kb.thread = NULL;
	}

	/* Answer all messages in 'calls' and 'dispatched_calls' queues. */
	ipc_cleanup_call_list(&TASK->kb.box, &TASK->kb.box.calls);
	ipc_cleanup_call_list(&TASK->kb.box, &TASK->kb.box.dispatched_calls);
}

/** Handle hangup message in kbox.
 *
 * @param call The IPC_M_PHONE_HUNGUP call structure.
 * @param last Output, the function stores @c true here if
 *             this was the last phone, @c false otherwise.
 *
 */
static void kbox_proc_phone_hungup(call_t *call, bool *last)
{
	/* Was it our debugger, who hung up? */
	if (call->sender == TASK->udebug.debugger) {
		/* Terminate debugging session (if any). */
		LOG("Terminate debugging session.");
		mutex_lock(&TASK->udebug.lock);
		udebug_task_cleanup(TASK);
		mutex_unlock(&TASK->udebug.lock);
	} else {
		LOG("Was not debugger.");
	}

	LOG("Continue with hangup message.");
	IPC_SET_RETVAL(call->data, 0);
	ipc_answer(&TASK->kb.box, call);

	mutex_lock(&TASK->kb.cleanup_lock);

	irq_spinlock_lock(&TASK->lock, true);
	irq_spinlock_lock(&TASK->kb.box.lock, false);
	if (list_empty(&TASK->kb.box.connected_phones)) {
		/*
		 * Last phone has been disconnected. Detach this thread so it
		 * gets freed and signal to the caller.
		 */

		/* Only detach kbox thread unless already terminating. */
		if (TASK->kb.finished == false) {
			/* Detach kbox thread so it gets freed from memory. */
			thread_detach(TASK->kb.thread);
			TASK->kb.thread = NULL;
		}

		LOG("Phone list is empty.");
		*last = true;
	} else
		*last = false;

	irq_spinlock_unlock(&TASK->kb.box.lock, false);
	irq_spinlock_unlock(&TASK->lock, true);

	mutex_unlock(&TASK->kb.cleanup_lock);
}

/** Implementing function for the kbox thread.
 *
 * This function listens for debug requests. It terminates
 * when all phones are disconnected from the kbox.
 *
 * @param arg Ignored.
 *
 */
static void kbox_thread_proc(void *arg)
{
	(void) arg;
	LOG("Starting.");
	bool done = false;

	while (!done) {
		call_t *call = ipc_wait_for_call(&TASK->kb.box, SYNCH_NO_TIMEOUT,
		    SYNCH_FLAGS_NONE);

		if (call == NULL)
			continue;  /* Try again. */

		switch (IPC_GET_IMETHOD(call->data)) {

		case IPC_M_DEBUG:
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

	LOG("Exiting.");
}

/** Connect phone to a task kernel-box specified by id.
 *
 * Note that this is not completely atomic. For optimisation reasons, the task
 * might start cleaning up kbox after the phone has been connected and before
 * a kbox thread has been created. This must be taken into account in the
 * cleanup code.
 *
 * @param[out] out_phone  Phone capability handle on success.
 * @return Error code.
 *
 */
errno_t ipc_connect_kbox(task_id_t taskid, cap_handle_t *out_phone)
{
	irq_spinlock_lock(&tasks_lock, true);

	task_t *task = task_find_by_id(taskid);
	if (task == NULL) {
		irq_spinlock_unlock(&tasks_lock, true);
		return ENOENT;
	}

	atomic_inc(&task->refcount);

	irq_spinlock_unlock(&tasks_lock, true);

	mutex_lock(&task->kb.cleanup_lock);

	if (atomic_predec(&task->refcount) == 0) {
		mutex_unlock(&task->kb.cleanup_lock);
		task_destroy(task);
		return ENOENT;
	}

	if (task->kb.finished) {
		mutex_unlock(&task->kb.cleanup_lock);
		return EINVAL;
	}

	/* Create a kbox thread if necessary. */
	if (task->kb.thread == NULL) {
		thread_t *kb_thread = thread_create(kbox_thread_proc, NULL, task,
		    THREAD_FLAG_NONE, "kbox");

		if (!kb_thread) {
			mutex_unlock(&task->kb.cleanup_lock);
			return ENOMEM;
		}

		task->kb.thread = kb_thread;
		thread_ready(kb_thread);
	}

	/* Allocate a new phone. */
	cap_handle_t phone_handle;
	errno_t rc = phone_alloc(TASK, true, &phone_handle, NULL);
	if (rc != EOK) {
		mutex_unlock(&task->kb.cleanup_lock);
		return rc;
	}

	kobject_t *phone_obj = kobject_get(TASK, phone_handle,
	    KOBJECT_TYPE_PHONE);
	/* Connect the newly allocated phone to the kbox */
	/* Hand over phone_obj's reference to ipc_phone_connect() */
	(void) ipc_phone_connect(phone_obj->phone, &task->kb.box);

	mutex_unlock(&task->kb.cleanup_lock);
	*out_phone = phone_handle;
	return EOK;
}

/** @}
 */
