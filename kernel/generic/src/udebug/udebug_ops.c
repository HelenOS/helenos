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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief	Udebug operations.
 *
 * Udebug operations on tasks and threads are implemented here. The
 * functions defined here are called from the udebug_ipc module
 * when servicing udebug IPC messages.
 */
 
#include <debug.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <errno.h>
#include <print.h>
#include <syscall/copy.h>
#include <ipc/ipc.h>
#include <udebug/udebug.h>
#include <udebug/udebug_ops.h>

/**
 * Prepare a thread for a debugging operation.
 *
 * Simply put, return thread t with t->udebug.lock held,
 * but only if it verifies all conditions.
 *
 * Specifically, verifies that thread t exists, is a userspace thread,
 * and belongs to the current task (TASK). Verifies, that the thread
 * is (or is not) go according to being_go (typically false).
 * It also locks t->udebug.lock, making sure that t->udebug.active
 * is true - that the thread is in a valid debugging session.
 *
 * With this verified and the t->udebug.lock mutex held, it is ensured
 * that the thread cannot leave the debugging session, let alone cease
 * to exist.
 *
 * In this function, holding the TASK->udebug.lock mutex prevents the
 * thread from leaving the debugging session, while relaxing from
 * the t->lock spinlock to the t->udebug.lock mutex.
 *
 * @param t		Pointer, need not at all be valid.
 * @param being_go	Required thread state.
 *
 * Returns EOK if all went well, or an error code otherwise.
 */
static int _thread_op_begin(thread_t *t, bool being_go)
{
	task_id_t taskid;
	ipl_t ipl;

	taskid = TASK->taskid;

	mutex_lock(&TASK->udebug.lock);

	/* thread_exists() must be called with threads_lock held */
	ipl = interrupts_disable();
	spinlock_lock(&threads_lock);

	if (!thread_exists(t)) {
		spinlock_unlock(&threads_lock);
		interrupts_restore(ipl);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}

	/* t->lock is enough to ensure the thread's existence */
	spinlock_lock(&t->lock);
	spinlock_unlock(&threads_lock);

	/* Verify that 't' is a userspace thread. */
	if ((t->flags & THREAD_FLAG_USPACE) == 0) {
		/* It's not, deny its existence */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}

	/* Verify debugging state. */
	if (t->udebug.active != true) {
		/* Not in debugging session or undesired GO state */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}

	/*
	 * Since the thread has active == true, TASK->udebug.lock
	 * is enough to ensure its existence and that active remains
	 * true.
	 */
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);

	/* Only mutex TASK->udebug.lock left. */
	
	/* Now verify that the thread belongs to the current task. */
	if (t->task != TASK) {
		/* No such thread belonging this task*/
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}

	/*
	 * Now we need to grab the thread's debug lock for synchronization
	 * of the threads stoppability/stop state.
	 */
	mutex_lock(&t->udebug.lock);

	/* The big task mutex is no longer needed. */
	mutex_unlock(&TASK->udebug.lock);

	if (t->udebug.go != being_go) {
		/* Not in debugging session or undesired GO state. */
		mutex_unlock(&t->udebug.lock);
		return EINVAL;
	}

	/* Only t->udebug.lock left. */

	return EOK;	/* All went well. */
}

/** End debugging operation on a thread. */
static void _thread_op_end(thread_t *t)
{
	mutex_unlock(&t->udebug.lock);
}

/** Begin debugging the current task.
 *
 * Initiates a debugging session for the current task (and its threads).
 * When the debugging session has started a reply will be sent to the
 * UDEBUG_BEGIN call. This may happen immediately in this function if
 * all the threads in this task are stoppable at the moment and in this
 * case the function returns 1.
 *
 * Otherwise the function returns 0 and the reply will be sent as soon as
 * all the threads become stoppable (i.e. they can be considered stopped).
 *
 * @param call	The BEGIN call we are servicing.
 * @return 	0 (OK, but not done yet), 1 (done) or negative error code.
 */
int udebug_begin(call_t *call)
{
	int reply;

	thread_t *t;
	link_t *cur;

	LOG("Debugging task %llu", TASK->taskid);
	mutex_lock(&TASK->udebug.lock);

	if (TASK->udebug.dt_state != UDEBUG_TS_INACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		return EBUSY;
	}

	TASK->udebug.dt_state = UDEBUG_TS_BEGINNING;
	TASK->udebug.begin_call = call;
	TASK->udebug.debugger = call->sender;

	if (TASK->udebug.not_stoppable_count == 0) {
		TASK->udebug.dt_state = UDEBUG_TS_ACTIVE;
		TASK->udebug.begin_call = NULL;
		reply = 1; /* immediate reply */
	} else {
		reply = 0; /* no reply */
	}
	
	/* Set udebug.active on all of the task's userspace threads. */

	for (cur = TASK->th_head.next; cur != &TASK->th_head; cur = cur->next) {
		t = list_get_instance(cur, thread_t, th_link);

		mutex_lock(&t->udebug.lock);
		if ((t->flags & THREAD_FLAG_USPACE) != 0)
			t->udebug.active = true;
		mutex_unlock(&t->udebug.lock);
	}

	mutex_unlock(&TASK->udebug.lock);
	return reply;
}

/** Finish debugging the current task.
 *
 * Closes the debugging session for the current task.
 * @return Zero on success or negative error code.
 */
int udebug_end(void)
{
	int rc;

	LOG("Task %" PRIu64, TASK->taskid);

	mutex_lock(&TASK->udebug.lock);
	rc = udebug_task_cleanup(TASK);
	mutex_unlock(&TASK->udebug.lock);

	return rc;
}

/** Set the event mask.
 *
 * Sets the event mask that determines which events are enabled.
 *
 * @param mask	Or combination of events that should be enabled.
 * @return	Zero on success or negative error code.
 */
int udebug_set_evmask(udebug_evmask_t mask)
{
	LOG("mask = 0x%x", mask);

	mutex_lock(&TASK->udebug.lock);

	if (TASK->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		return EINVAL;
	}

	TASK->udebug.evmask = mask;
	mutex_unlock(&TASK->udebug.lock);

	return 0;
}

/** Give thread GO.
 *
 * Upon recieving a go message, the thread is given GO. Being GO
 * means the thread is allowed to execute userspace code (until
 * a debugging event or STOP occurs, at which point the thread loses GO.
 *
 * @param t	The thread to operate on (unlocked and need not be valid).
 * @param call	The GO call that we are servicing.
 */
int udebug_go(thread_t *t, call_t *call)
{
	int rc;

	/* On success, this will lock t->udebug.lock. */
	rc = _thread_op_begin(t, false);
	if (rc != EOK) {
		return rc;
	}

	t->udebug.go_call = call;
	t->udebug.go = true;
	t->udebug.cur_event = 0;	/* none */

	/*
	 * Neither t's lock nor threads_lock may be held during wakeup.
	 */
	waitq_wakeup(&t->udebug.go_wq, WAKEUP_FIRST);

	_thread_op_end(t);

	return 0;
}

/** Stop a thread (i.e. take its GO away)
 *
 * Generates a STOP event as soon as the thread becomes stoppable (i.e.
 * can be considered stopped).
 *
 * @param t	The thread to operate on (unlocked and need not be valid).
 * @param call	The GO call that we are servicing.
 */
int udebug_stop(thread_t *t, call_t *call)
{
	int rc;

	LOG("udebug_stop()");

	/*
	 * On success, this will lock t->udebug.lock. Note that this makes sure
	 * the thread is not stopped.
	 */
	rc = _thread_op_begin(t, true);
	if (rc != EOK) {
		return rc;
	}

	/* Take GO away from the thread. */
	t->udebug.go = false;

	if (t->udebug.stoppable != true) {
		/* Answer will be sent when the thread becomes stoppable. */
		_thread_op_end(t);
		return 0;
	}

	/*
	 * Answer GO call.
	 */

	/* Make sure nobody takes this call away from us. */
	call = t->udebug.go_call;
	t->udebug.go_call = NULL;

	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, UDEBUG_EVENT_STOP);

	THREAD->udebug.cur_event = UDEBUG_EVENT_STOP;

	_thread_op_end(t);

	mutex_lock(&TASK->udebug.lock);
	ipc_answer(&TASK->answerbox, call);
	mutex_unlock(&TASK->udebug.lock);

	return 0;
}

/** Read the list of userspace threads in the current task.
 *
 * The list takes the form of a sequence of thread hashes (i.e. the pointers
 * to thread structures). A buffer of size @a buf_size is allocated and
 * a pointer to it written to @a buffer. The sequence of hashes is written
 * into this buffer.
 *
 * If the sequence is longer than @a buf_size bytes, only as much hashes
 * as can fit are copied. The number of thread hashes copied is stored
 * in @a n.
 *
 * The rationale for having @a buf_size is that this function is only
 * used for servicing the THREAD_READ message, which always specifies
 * a maximum size for the userspace buffer.
 *
 * @param buffer	The buffer for storing thread hashes.
 * @param buf_size	Buffer size in bytes.
 * @param n		The actual number of hashes copied will be stored here.
 */
int udebug_thread_read(void **buffer, size_t buf_size, size_t *n)
{
	thread_t *t;
	link_t *cur;
	unative_t tid;
	unsigned copied_ids;
	ipl_t ipl;
	unative_t *id_buffer;
	int flags;
	size_t max_ids;

	LOG("udebug_thread_read()");

	/* Allocate a buffer to hold thread IDs */
	id_buffer = malloc(buf_size, 0);

	mutex_lock(&TASK->udebug.lock);

	/* Verify task state */
	if (TASK->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		return EINVAL;
	}

	ipl = interrupts_disable();
	spinlock_lock(&TASK->lock);
	/* Copy down the thread IDs */

	max_ids = buf_size / sizeof(unative_t);
	copied_ids = 0;

	/* FIXME: make sure the thread isn't past debug shutdown... */
	for (cur = TASK->th_head.next; cur != &TASK->th_head; cur = cur->next) {
		/* Do not write past end of buffer */
		if (copied_ids >= max_ids) break;

		t = list_get_instance(cur, thread_t, th_link);

		spinlock_lock(&t->lock);
		flags = t->flags;
		spinlock_unlock(&t->lock);

		/* Not interested in kernel threads. */
		if ((flags & THREAD_FLAG_USPACE) != 0) {
			/* Using thread struct pointer as identification hash */
			tid = (unative_t) t;
			id_buffer[copied_ids++] = tid;
		}
	}

	spinlock_unlock(&TASK->lock);
	interrupts_restore(ipl);

	mutex_unlock(&TASK->udebug.lock);

	*buffer = id_buffer;
	*n = copied_ids * sizeof(unative_t);

	return 0;
}

/** Read the arguments of a system call.
 *
 * The arguments of the system call being being executed are copied
 * to an allocated buffer and a pointer to it is written to @a buffer.
 * The size of the buffer is exactly such that it can hold the maximum number
 * of system-call arguments.
 *
 * Unless the thread is currently blocked in a SYSCALL_B or SYSCALL_E event,
 * this function will fail with an EINVAL error code.
 *
 * @param buffer	The buffer for storing thread hashes.
 */
int udebug_args_read(thread_t *t, void **buffer)
{
	int rc;
	unative_t *arg_buffer;

	/* Prepare a buffer to hold the arguments. */
	arg_buffer = malloc(6 * sizeof(unative_t), 0);

	/* On success, this will lock t->udebug.lock. */
	rc = _thread_op_begin(t, false);
	if (rc != EOK) {
		return rc;
	}

	/* Additionally we need to verify that we are inside a syscall. */
	if (t->udebug.cur_event != UDEBUG_EVENT_SYSCALL_B &&
	    t->udebug.cur_event != UDEBUG_EVENT_SYSCALL_E) {
		_thread_op_end(t);
		return EINVAL;
	}

	/* Copy to a local buffer before releasing the lock. */
	memcpy(arg_buffer, t->udebug.syscall_args, 6 * sizeof(unative_t));

	_thread_op_end(t);

	*buffer = arg_buffer;
	return 0;
}

/** Read the memory of the debugged task.
 *
 * Reads @a n bytes from the address space of the debugged task, starting
 * from @a uspace_addr. The bytes are copied into an allocated buffer
 * and a pointer to it is written into @a buffer.
 *
 * @param uspace_addr	Address from where to start reading.
 * @param n		Number of bytes to read.
 * @param buffer	For storing a pointer to the allocated buffer.
 */
int udebug_mem_read(unative_t uspace_addr, size_t n, void **buffer)
{
	void *data_buffer;
	int rc;

	/* Verify task state */
	mutex_lock(&TASK->udebug.lock);

	if (TASK->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		return EBUSY;
	}

	data_buffer = malloc(n, 0);

	/* NOTE: this is not strictly from a syscall... but that shouldn't
	 * be a problem */
	rc = copy_from_uspace(data_buffer, (void *)uspace_addr, n);
	mutex_unlock(&TASK->udebug.lock);

	if (rc != 0) return rc;

	*buffer = data_buffer;
	return 0;
}

/** @}
 */
