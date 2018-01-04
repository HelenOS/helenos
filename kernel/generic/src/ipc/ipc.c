/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/* Lock ordering
 *
 * First the answerbox, then the phone.
 */

#include <assert.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <synch/waitq.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <abi/ipc/methods.h>
#include <ipc/kbox.h>
#include <ipc/event.h>
#include <ipc/sysipc_ops.h>
#include <ipc/sysipc_priv.h>
#include <errno.h>
#include <mm/slab.h>
#include <arch.h>
#include <proc/task.h>
#include <mem.h>
#include <print.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch/interrupt.h>
#include <ipc/irq.h>
#include <cap/cap.h>

static void ipc_forget_call(call_t *);

/** Open channel that is assigned automatically to new tasks */
answerbox_t *ipc_phone_0 = NULL;

static slab_cache_t *call_cache;
static slab_cache_t *answerbox_cache;

slab_cache_t *phone_cache = NULL; 

/** Initialize a call structure.
 *
 * @param call Call structure to be initialized.
 *
 */
static void _ipc_call_init(call_t *call)
{
	memsetb(call, sizeof(*call), 0);
	spinlock_initialize(&call->forget_lock, "forget_lock");
	call->active = false;
	call->forget = false;
	call->sender = NULL;
	call->callerbox = NULL;
	call->buffer = NULL;
}

static void call_destroy(void *arg)
{
	call_t *call = (call_t *) arg;

	if (call->buffer)
		free(call->buffer);
	if (call->caller_phone)
		kobject_put(call->caller_phone->kobject);
	slab_free(call_cache, call);
}

static kobject_ops_t call_kobject_ops = {
	.destroy = call_destroy
};

/** Allocate and initialize a call structure.
 *
 * The call is initialized, so that the reply will be directed to
 * TASK->answerbox.
 *
 * @param flags Parameters for slab_alloc (e.g FRAME_ATOMIC).
 *
 * @return If flags permit it, return NULL, or initialized kernel
 *         call structure with one reference.
 *
 */
call_t *ipc_call_alloc(unsigned int flags)
{
	call_t *call = slab_alloc(call_cache, flags);
	if (!call)
		return NULL;
	kobject_t *kobj = (kobject_t *) malloc(sizeof(kobject_t), flags);
	if (!kobj) {
		slab_free(call_cache, call);
		return NULL;
	}

	_ipc_call_init(call);
	kobject_initialize(kobj, KOBJECT_TYPE_CALL, call, &call_kobject_ops);
	call->kobject = kobj;
	
	return call;
}

/** Initialize an answerbox structure.
 *
 * @param box  Answerbox structure to be initialized.
 * @param task Task to which the answerbox belongs.
 *
 */
void ipc_answerbox_init(answerbox_t *box, task_t *task)
{
	irq_spinlock_initialize(&box->lock, "ipc.box.lock");
	irq_spinlock_initialize(&box->irq_lock, "ipc.box.irqlock");
	waitq_initialize(&box->wq);
	list_initialize(&box->connected_phones);
	list_initialize(&box->calls);
	list_initialize(&box->dispatched_calls);
	list_initialize(&box->answers);
	list_initialize(&box->irq_notifs);
	box->task = task;
}

/** Connect a phone to an answerbox.
 *
 * This function must be passed a reference to phone->kobject.
 *
 * @param phone  Initialized phone structure.
 * @param box    Initialized answerbox structure.
 * @return       True if the phone was connected, false otherwise.
 */
bool ipc_phone_connect(phone_t *phone, answerbox_t *box)
{
	bool active;

	mutex_lock(&phone->lock);
	irq_spinlock_lock(&box->lock, true);

	active = box->active;
	if (active) {
		phone->state = IPC_PHONE_CONNECTED;
		phone->callee = box;
		/* Pass phone->kobject reference to box->connected_phones */
		list_append(&phone->link, &box->connected_phones);
	}

	irq_spinlock_unlock(&box->lock, true);
	mutex_unlock(&phone->lock);

	if (!active) {
		/* We still have phone->kobject's reference; drop it */
		kobject_put(phone->kobject);
	}

	return active;
}

/** Initialize a phone structure.
 *
 * @param phone Phone structure to be initialized.
 * @param caller Owning task.
 *
 */
void ipc_phone_init(phone_t *phone, task_t *caller)
{
	mutex_initialize(&phone->lock, MUTEX_PASSIVE);
	phone->caller = caller;
	phone->callee = NULL;
	phone->state = IPC_PHONE_FREE;
	atomic_set(&phone->active_calls, 0);
	phone->kobject = NULL;
}

/** Helper function to facilitate synchronous calls.
 *
 * @param phone   Destination kernel phone structure.
 * @param request Call structure with request.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t ipc_call_sync(phone_t *phone, call_t *request)
{
	answerbox_t *mybox = slab_alloc(answerbox_cache, 0);
	ipc_answerbox_init(mybox, TASK);
	
	/* We will receive data in a special box. */
	request->callerbox = mybox;
	
	errno_t rc = ipc_call(phone, request);
	if (rc != EOK) {
		slab_free(answerbox_cache, mybox);
		return rc;
	}

	call_t *answer = ipc_wait_for_call(mybox, SYNCH_NO_TIMEOUT,
	    SYNCH_FLAGS_INTERRUPTIBLE);
	if (!answer) {

		/*
		 * The sleep was interrupted.
		 *
		 * There are two possibilities now:
		 * 1) the call gets answered before we manage to forget it
		 * 2) we manage to forget the call before it gets answered
		 */

		spinlock_lock(&request->forget_lock);
		spinlock_lock(&TASK->active_calls_lock);

		assert(!request->forget);

		bool answered = !request->active;
		if (!answered) {
			/*
			 * The call is not yet answered and we won the race to
			 * forget it.
			 */
			ipc_forget_call(request);	/* releases locks */
			rc = EINTR;
			
		} else {
			spinlock_unlock(&TASK->active_calls_lock);
			spinlock_unlock(&request->forget_lock);
		}

		if (answered) {
			/*
			 * The other side won the race to answer the call.
			 * It is safe to wait for the answer uninterruptibly
			 * now.
			 */
			answer = ipc_wait_for_call(mybox, SYNCH_NO_TIMEOUT,
			    SYNCH_FLAGS_NONE);
		}
	}
	assert(!answer || request == answer);
	
	slab_free(answerbox_cache, mybox);
	return rc;
}

/** Answer a message which was not dispatched and is not listed in any queue.
 *
 * @param call       Call structure to be answered.
 * @param selflocked If true, then TASK->answebox is locked.
 *
 */
void _ipc_answer_free_call(call_t *call, bool selflocked)
{
	/* Count sent answer */
	irq_spinlock_lock(&TASK->lock, true);
	TASK->ipc_info.answer_sent++;
	irq_spinlock_unlock(&TASK->lock, true);

	spinlock_lock(&call->forget_lock);
	if (call->forget) {
		/* This is a forgotten call and call->sender is not valid. */
		spinlock_unlock(&call->forget_lock);
		kobject_put(call->kobject);
		return;
	} else {
		/*
		 * If the call is still active, i.e. it was answered
		 * in a non-standard way, remove the call from the
		 * sender's active call list.
		 */
		if (call->active) {
			spinlock_lock(&call->sender->active_calls_lock);
			list_remove(&call->ta_link);
			spinlock_unlock(&call->sender->active_calls_lock);
		}
	}
	spinlock_unlock(&call->forget_lock);

	answerbox_t *callerbox = call->callerbox ? call->callerbox :
	    &call->sender->answerbox;
	bool do_lock = ((!selflocked) || (callerbox != &TASK->answerbox));
	
	call->flags |= IPC_CALL_ANSWERED;
	
	call->data.task_id = TASK->taskid;
	
	if (do_lock)
		irq_spinlock_lock(&callerbox->lock, true);
	
	list_append(&call->ab_link, &callerbox->answers);
	
	if (do_lock)
		irq_spinlock_unlock(&callerbox->lock, true);
	
	waitq_wakeup(&callerbox->wq, WAKEUP_FIRST);
}

/** Answer a message which is in a callee queue.
 *
 * @param box  Answerbox that is answering the message.
 * @param call Modified request that is being sent back.
 *
 */
void ipc_answer(answerbox_t *box, call_t *call)
{
	/* Remove from active box */
	irq_spinlock_lock(&box->lock, true);
	list_remove(&call->ab_link);
	irq_spinlock_unlock(&box->lock, true);
	
	/* Send back answer */
	_ipc_answer_free_call(call, false);
}

static void _ipc_call_actions_internal(phone_t *phone, call_t *call,
    bool preforget)
{
	task_t *caller = phone->caller;

	call->caller_phone = phone;
	kobject_add_ref(phone->kobject);

	if (preforget) {
		call->forget = true;
	} else {
		atomic_inc(&phone->active_calls);
		call->sender = caller;
		call->active = true;
		spinlock_lock(&caller->active_calls_lock);
		list_append(&call->ta_link, &caller->active_calls);
		spinlock_unlock(&caller->active_calls_lock);
	}

	call->data.phone = phone;
	call->data.task_id = caller->taskid;
}

/** Simulate sending back a message.
 *
 * Most errors are better handled by forming a normal backward
 * message and sending it as a normal answer.
 *
 * @param phone Phone structure the call should appear to come from.
 * @param call  Call structure to be answered.
 * @param err   Return value to be used for the answer.
 *
 */
void ipc_backsend_err(phone_t *phone, call_t *call, errno_t err)
{
	_ipc_call_actions_internal(phone, call, false);
	IPC_SET_RETVAL(call->data, err);
	_ipc_answer_free_call(call, false);
}

/** Unsafe unchecking version of ipc_call.
 *
 * @param phone     Phone structure the call comes from.
 * @param box       Destination answerbox structure.
 * @param call      Call structure with request.
 * @param preforget If true, the call will be delivered already forgotten.
 *
 */
static void _ipc_call(phone_t *phone, answerbox_t *box, call_t *call,
    bool preforget)
{
	task_t *caller = phone->caller;

	/* Count sent ipc call */
	irq_spinlock_lock(&caller->lock, true);
	caller->ipc_info.call_sent++;
	irq_spinlock_unlock(&caller->lock, true);
	
	if (!(call->flags & IPC_CALL_FORWARDED))
		_ipc_call_actions_internal(phone, call, preforget);
	
	irq_spinlock_lock(&box->lock, true);
	list_append(&call->ab_link, &box->calls);
	irq_spinlock_unlock(&box->lock, true);
	
	waitq_wakeup(&box->wq, WAKEUP_FIRST);
}

/** Send an asynchronous request using a phone to an answerbox.
 *
 * @param phone Phone structure the call comes from and which is
 *              connected to the destination answerbox.
 * @param call  Call structure with request.
 *
 * @return Return 0 on success, ENOENT on error.
 *
 */
errno_t ipc_call(phone_t *phone, call_t *call)
{
	mutex_lock(&phone->lock);
	if (phone->state != IPC_PHONE_CONNECTED) {
		mutex_unlock(&phone->lock);
		if (!(call->flags & IPC_CALL_FORWARDED)) {
			if (phone->state == IPC_PHONE_HUNGUP)
				ipc_backsend_err(phone, call, EHANGUP);
			else
				ipc_backsend_err(phone, call, ENOENT);
		}
		
		return ENOENT;
	}
	
	answerbox_t *box = phone->callee;
	_ipc_call(phone, box, call, false);
	
	mutex_unlock(&phone->lock);
	return 0;
}

/** Disconnect phone from answerbox.
 *
 * This call leaves the phone in the HUNGUP state. The change to 'free' is done
 * lazily later.
 *
 * @param phone Phone structure to be hung up.
 *
 * @return EOK if the phone is disconnected.
 * @return EINVAL if the phone was already disconnected.
 *
 */
errno_t ipc_phone_hangup(phone_t *phone)
{
	mutex_lock(&phone->lock);
	if (phone->state == IPC_PHONE_FREE ||
	    phone->state == IPC_PHONE_HUNGUP ||
	    phone->state == IPC_PHONE_CONNECTING) {
		mutex_unlock(&phone->lock);
		return EINVAL;
	}
	
	answerbox_t *box = phone->callee;
	if (phone->state != IPC_PHONE_SLAMMED) {
		/* Remove myself from answerbox */
		irq_spinlock_lock(&box->lock, true);
		list_remove(&phone->link);
		irq_spinlock_unlock(&box->lock, true);

		/* Drop the answerbox reference */
		kobject_put(phone->kobject);
		
		call_t *call = ipc_call_alloc(0);
		IPC_SET_IMETHOD(call->data, IPC_M_PHONE_HUNGUP);
		call->request_method = IPC_M_PHONE_HUNGUP;
		call->flags |= IPC_CALL_DISCARD_ANSWER;
		_ipc_call(phone, box, call, false);
	}
	
	phone->state = IPC_PHONE_HUNGUP;
	mutex_unlock(&phone->lock);
	
	return EOK;
}

/** Forwards call from one answerbox to another one.
 *
 * @param call     Call structure to be redirected.
 * @param newphone Phone structure to target answerbox.
 * @param oldbox   Old answerbox structure.
 * @param mode     Flags that specify mode of the forward operation.
 *
 * @return 0 if forwarding succeeded or an error code if
 *         there was an error.
 *
 * The return value serves only as an information for the forwarder,
 * the original caller is notified automatically with EFORWARD.
 *
 */
errno_t ipc_forward(call_t *call, phone_t *newphone, answerbox_t *oldbox,
    unsigned int mode)
{
	/* Count forwarded calls */
	irq_spinlock_lock(&TASK->lock, true);
	TASK->ipc_info.forwarded++;
	irq_spinlock_pass(&TASK->lock, &oldbox->lock);
	list_remove(&call->ab_link);
	irq_spinlock_unlock(&oldbox->lock, true);
	
	if (mode & IPC_FF_ROUTE_FROM_ME) {
		call->data.phone = newphone;
		call->data.task_id = TASK->taskid;
	}
	
	return ipc_call(newphone, call);
}


/** Wait for a phone call.
 *
 * @param box   Answerbox expecting the call.
 * @param usec  Timeout in microseconds. See documentation for
 *              waitq_sleep_timeout() for decription of its special
 *              meaning.
 * @param flags Select mode of sleep operation. See documentation for
 *              waitq_sleep_timeout() for description of its special
 *              meaning.
 *
 * @return Recived call structure or NULL.
 *
 * To distinguish between a call and an answer, have a look at call->flags.
 *
 */
call_t *ipc_wait_for_call(answerbox_t *box, uint32_t usec, unsigned int flags)
{
	call_t *request;
	uint64_t irq_cnt = 0;
	uint64_t answer_cnt = 0;
	uint64_t call_cnt = 0;
	errno_t rc;
	
restart:
	rc = waitq_sleep_timeout(&box->wq, usec, flags, NULL);
	if (rc != EOK)
		return NULL;
	
	irq_spinlock_lock(&box->lock, true);
	if (!list_empty(&box->irq_notifs)) {
		/* Count received IRQ notification */
		irq_cnt++;
		
		irq_spinlock_lock(&box->irq_lock, false);
		
		request = list_get_instance(list_first(&box->irq_notifs),
		    call_t, ab_link);
		list_remove(&request->ab_link);
		
		irq_spinlock_unlock(&box->irq_lock, false);
	} else if (!list_empty(&box->answers)) {
		/* Count received answer */
		answer_cnt++;
		
		/* Handle asynchronous answers */
		request = list_get_instance(list_first(&box->answers),
		    call_t, ab_link);
		list_remove(&request->ab_link);
		atomic_dec(&request->caller_phone->active_calls);
	} else if (!list_empty(&box->calls)) {
		/* Count received call */
		call_cnt++;
		
		/* Handle requests */
		request = list_get_instance(list_first(&box->calls),
		    call_t, ab_link);
		list_remove(&request->ab_link);
		
		/* Append request to dispatch queue */
		list_append(&request->ab_link, &box->dispatched_calls);
	} else {
		/* This can happen regularly after ipc_cleanup */
		irq_spinlock_unlock(&box->lock, true);
		goto restart;
	}
	
	irq_spinlock_pass(&box->lock, &TASK->lock);
	
	TASK->ipc_info.irq_notif_received += irq_cnt;
	TASK->ipc_info.answer_received += answer_cnt;
	TASK->ipc_info.call_received += call_cnt;
	
	irq_spinlock_unlock(&TASK->lock, true);
	
	return request;
}

/** Answer all calls from list with EHANGUP answer.
 *
 * @param box Answerbox with the list.
 * @param lst Head of the list to be cleaned up.
 */
void ipc_cleanup_call_list(answerbox_t *box, list_t *lst)
{
	irq_spinlock_lock(&box->lock, true);
	while (!list_empty(lst)) {
		call_t *call = list_get_instance(list_first(lst), call_t,
		    ab_link);
		
		list_remove(&call->ab_link);

		irq_spinlock_unlock(&box->lock, true);

		if (lst == &box->calls)
			SYSIPC_OP(request_process, call, box);

		ipc_data_t old = call->data;
		IPC_SET_RETVAL(call->data, EHANGUP);
		answer_preprocess(call, &old);
		_ipc_answer_free_call(call, true);

		irq_spinlock_lock(&box->lock, true);
	}
	irq_spinlock_unlock(&box->lock, true);
}

/** Disconnects all phones connected to an answerbox.
 *
 * @param box        Answerbox to disconnect phones from.
 * @param notify_box If true, the answerbox will get a hangup message for
 *                   each disconnected phone.
 *
 */
void ipc_answerbox_slam_phones(answerbox_t *box, bool notify_box)
{
	phone_t *phone;
	DEADLOCK_PROBE_INIT(p_phonelck);
	
	/* Disconnect all phones connected to our answerbox */
restart_phones:
	irq_spinlock_lock(&box->lock, true);
	while (!list_empty(&box->connected_phones)) {
		phone = list_get_instance(list_first(&box->connected_phones),
		    phone_t, link);
		if (mutex_trylock(&phone->lock) != EOK) {
			irq_spinlock_unlock(&box->lock, true);
			DEADLOCK_PROBE(p_phonelck, DEADLOCK_THRESHOLD);
			goto restart_phones;
		}
		
		/* Disconnect phone */
		assert(phone->state == IPC_PHONE_CONNECTED);
		
		list_remove(&phone->link);
		phone->state = IPC_PHONE_SLAMMED;
		
		if (notify_box) {
			task_hold(phone->caller);
			mutex_unlock(&phone->lock);
			irq_spinlock_unlock(&box->lock, true);

			/*
			 * Send one call to the answerbox for each phone.
			 * Used to make sure the kbox thread wakes up after
			 * the last phone has been disconnected. The call is
			 * forgotten upon sending, so the "caller" may cease
			 * to exist as soon as we release it.
			 */
			call_t *call = ipc_call_alloc(0);
			IPC_SET_IMETHOD(call->data, IPC_M_PHONE_HUNGUP);
			call->request_method = IPC_M_PHONE_HUNGUP;
			call->flags |= IPC_CALL_DISCARD_ANSWER;
			_ipc_call(phone, box, call, true);

			task_release(phone->caller);

			kobject_put(phone->kobject);
			
			/* Must start again */
			goto restart_phones;
		}
		
		mutex_unlock(&phone->lock);
		kobject_put(phone->kobject);
	}
	
	irq_spinlock_unlock(&box->lock, true);
}

static void ipc_forget_call(call_t *call)
{
	assert(spinlock_locked(&TASK->active_calls_lock));
	assert(spinlock_locked(&call->forget_lock));

	/*
	 * Forget the call and donate it to the task which holds up the answer.
	 */

	call->forget = true;
	call->sender = NULL;
	list_remove(&call->ta_link);

	/*
	 * The call may be freed by _ipc_answer_free_call() before we are done
	 * with it; to avoid working with a destroyed call_t structure, we
	 * must hold a reference to it.
	 */
	kobject_add_ref(call->kobject);

	spinlock_unlock(&call->forget_lock);
	spinlock_unlock(&TASK->active_calls_lock);

	atomic_dec(&call->caller_phone->active_calls);

	SYSIPC_OP(request_forget, call);

	kobject_put(call->kobject);
}

static void ipc_forget_all_active_calls(void)
{
	call_t *call;

restart:
	spinlock_lock(&TASK->active_calls_lock);
	if (list_empty(&TASK->active_calls)) {
		/*
		 * We are done, there are no more active calls.
		 * Nota bene: there may still be answers waiting for pick up.
		 */
		spinlock_unlock(&TASK->active_calls_lock);
		return;	
	}
	
	call = list_get_instance(list_first(&TASK->active_calls), call_t,
	    ta_link);

	if (!spinlock_trylock(&call->forget_lock)) {
		/*
		 * Avoid deadlock and let async_answer() or
		 *  _ipc_answer_free_call() win the race to dequeue the first
		 * call on the list.
		 */
		spinlock_unlock(&TASK->active_calls_lock);
		goto restart;
	}

	ipc_forget_call(call);

	goto restart;
}

static bool phone_cap_wait_cb(cap_t *cap, void *arg)
{
	phone_t *phone = cap->kobject->phone;
	bool *restart = (bool *) arg;

	mutex_lock(&phone->lock);
	if ((phone->state == IPC_PHONE_HUNGUP) &&
	    (atomic_get(&phone->active_calls) == 0)) {
		phone->state = IPC_PHONE_FREE;
		phone->callee = NULL;
	}

	/*
	 * We might have had some IPC_PHONE_CONNECTING phones at the beginning
	 * of ipc_cleanup(). Depending on whether these were forgotten or
	 * answered, they will eventually enter the IPC_PHONE_FREE or
	 * IPC_PHONE_CONNECTED states, respectively.  In the latter case, the
	 * other side may slam the open phones at any time, in which case we
	 * will get an IPC_PHONE_SLAMMED phone.
	 */
	if ((phone->state == IPC_PHONE_CONNECTED) ||
	    (phone->state == IPC_PHONE_SLAMMED)) {
		mutex_unlock(&phone->lock);
		ipc_phone_hangup(phone);
		/*
		 * Now there may be one extra active call, which needs to be
		 * forgotten.
		 */
		ipc_forget_all_active_calls();
		*restart = true;
		return false;
	}

	/*
	 * If the hangup succeeded, it has sent a HANGUP message, the IPC is now
	 * in HUNGUP state, we wait for the reply to come
	 */
	if (phone->state != IPC_PHONE_FREE) {
		mutex_unlock(&phone->lock);
		return false;
	}

	mutex_unlock(&phone->lock);
	return true;
}

/** Wait for all answers to asynchronous calls to arrive. */
static void ipc_wait_for_all_answered_calls(void)
{
	call_t *call;
	bool restart;

restart:
	/*
	 * Go through all phones, until they are all free.
	 * Locking is needed as there may be connection handshakes in progress.
	 */
	restart = false;
	if (caps_apply_to_kobject_type(TASK, KOBJECT_TYPE_PHONE,
	    phone_cap_wait_cb, &restart)) {
		/* Got into cleanup */
		return;
	}
	if (restart)
		goto restart;
	
	call = ipc_wait_for_call(&TASK->answerbox, SYNCH_NO_TIMEOUT,
	    SYNCH_FLAGS_NONE);
	assert(call->flags & (IPC_CALL_ANSWERED | IPC_CALL_NOTIF));

	SYSIPC_OP(answer_process, call);

	kobject_put(call->kobject);
	goto restart;
}

static bool phone_cap_cleanup_cb(cap_t *cap, void *arg)
{
	ipc_phone_hangup(cap->kobject->phone);
	kobject_t *kobj = cap_unpublish(cap->task, cap->handle,
	    KOBJECT_TYPE_PHONE);
	kobject_put(kobj);
	cap_free(cap->task, cap->handle);
	return true;
}

static bool irq_cap_cleanup_cb(cap_t *cap, void *arg)
{
	ipc_irq_unsubscribe(&TASK->answerbox, cap->handle);
	return true;
}

static bool call_cap_cleanup_cb(cap_t *cap, void *arg)
{
	/*
	 * Here we just free the capability and release the kobject.
	 * The kernel answers the remaining calls elsewhere in ipc_cleanup().
	 */
	kobject_t *kobj = cap_unpublish(cap->task, cap->handle,
	    KOBJECT_TYPE_CALL);
	kobject_put(kobj);
	cap_free(cap->task, cap->handle);
	return true;
}

/** Clean up all IPC communication of the current task.
 *
 * Note: ipc_hangup sets returning answerbox to TASK->answerbox, you
 * have to change it as well if you want to cleanup other tasks than TASK.
 *
 */
void ipc_cleanup(void)
{
	/*
	 * Mark the answerbox as inactive.
	 *
	 * The main purpose for doing this is to prevent any pending callback
	 * connections from getting established beyond this point.
	 */
	irq_spinlock_lock(&TASK->answerbox.lock, true);
	TASK->answerbox.active = false;
	irq_spinlock_unlock(&TASK->answerbox.lock, true);

	/* Disconnect all our phones ('ipc_phone_hangup') */
	caps_apply_to_kobject_type(TASK, KOBJECT_TYPE_PHONE,
	    phone_cap_cleanup_cb, NULL);
	
	/* Unsubscribe from any event notifications. */
	event_cleanup_answerbox(&TASK->answerbox);
	
	/* Disconnect all connected IRQs */
	caps_apply_to_kobject_type(TASK, KOBJECT_TYPE_IRQ, irq_cap_cleanup_cb,
	    NULL);
	
	/* Disconnect all phones connected to our regular answerbox */
	ipc_answerbox_slam_phones(&TASK->answerbox, false);
	
#ifdef CONFIG_UDEBUG
	/* Clean up kbox thread and communications */
	ipc_kbox_cleanup();
#endif

	/* Destroy all call capabilities */
	caps_apply_to_kobject_type(TASK, KOBJECT_TYPE_CALL, call_cap_cleanup_cb,
	    NULL);
	
	/* Answer all messages in 'calls' and 'dispatched_calls' queues */
	ipc_cleanup_call_list(&TASK->answerbox, &TASK->answerbox.calls);
	ipc_cleanup_call_list(&TASK->answerbox,
	    &TASK->answerbox.dispatched_calls);
 	
	ipc_forget_all_active_calls();
	ipc_wait_for_all_answered_calls();
}

/** Initilize IPC subsystem
 *
 */
void ipc_init(void)
{
	call_cache = slab_cache_create("call_t", sizeof(call_t), 0, NULL,
	    NULL, 0);
	phone_cache = slab_cache_create("phone_t", sizeof(phone_t), 0, NULL,
	    NULL, 0);
	answerbox_cache = slab_cache_create("answerbox_t", sizeof(answerbox_t),
	    0, NULL, NULL, 0);
}


static void ipc_print_call_list(list_t *list)
{
	list_foreach(*list, ab_link, call_t, call) {
#ifdef __32_BITS__
		printf("%10p ", call);
#endif
		
#ifdef __64_BITS__
		printf("%18p ", call);
#endif
		
		spinlock_lock(&call->forget_lock);

		printf("%-8" PRIun " %-6" PRIun " %-6" PRIun " %-6" PRIun
		    " %-6" PRIun " %-6" PRIun " %-7x",
		    IPC_GET_IMETHOD(call->data), IPC_GET_ARG1(call->data),
		    IPC_GET_ARG2(call->data), IPC_GET_ARG3(call->data),
		    IPC_GET_ARG4(call->data), IPC_GET_ARG5(call->data),
		    call->flags);

		if (call->forget) {
			printf(" ? (call forgotten)\n");
		} else {
			printf(" %" PRIu64 " (%s)\n",
			    call->sender->taskid, call->sender->name);
		}

		spinlock_unlock(&call->forget_lock);
	}
}

static bool print_task_phone_cb(cap_t *cap, void *arg)
{
	phone_t *phone = cap->kobject->phone;

	mutex_lock(&phone->lock);
	if (phone->state != IPC_PHONE_FREE) {
		printf("%-11d %7" PRIun " ", cap->handle,
		    atomic_get(&phone->active_calls));
		
		switch (phone->state) {
		case IPC_PHONE_CONNECTING:
			printf("connecting");
			break;
		case IPC_PHONE_CONNECTED:
			printf("connected to %" PRIu64 " (%s)",
			    phone->callee->task->taskid,
			    phone->callee->task->name);
			break;
		case IPC_PHONE_SLAMMED:
			printf("slammed by %p", phone->callee);
			break;
		case IPC_PHONE_HUNGUP:
			printf("hung up by %p", phone->callee);
			break;
		default:
			break;
		}
		
		printf("\n");
	}
	mutex_unlock(&phone->lock);

	return true;
}

/** List answerbox contents.
 *
 * @param taskid Task ID.
 *
 */
void ipc_print_task(task_id_t taskid)
{
	irq_spinlock_lock(&tasks_lock, true);
	task_t *task = task_find_by_id(taskid);
	if (!task) {
		irq_spinlock_unlock(&tasks_lock, true);
		return;
	}
	task_hold(task);
	irq_spinlock_unlock(&tasks_lock, true);
	
	printf("[phone cap] [calls] [state\n");
	
	caps_apply_to_kobject_type(task, KOBJECT_TYPE_PHONE,
	    print_task_phone_cb, NULL);
	
	irq_spinlock_lock(&task->lock, true);
	irq_spinlock_lock(&task->answerbox.lock, false);
	
#ifdef __32_BITS__
	printf("[call id ] [method] [arg1] [arg2] [arg3] [arg4] [arg5]"
	    " [flags] [sender\n");
#endif
	
#ifdef __64_BITS__
	printf("[call id         ] [method] [arg1] [arg2] [arg3] [arg4]"
	    " [arg5] [flags] [sender\n");
#endif
	
	printf(" --- incomming calls ---\n");
	ipc_print_call_list(&task->answerbox.calls);
	printf(" --- dispatched calls ---\n");
	ipc_print_call_list(&task->answerbox.dispatched_calls);
	printf(" --- incoming answers ---\n");
	ipc_print_call_list(&task->answerbox.answers);
	
	irq_spinlock_unlock(&task->answerbox.lock, false);
	irq_spinlock_unlock(&task->lock, true);

	task_release(task);
}

/** @}
 */
