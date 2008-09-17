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
 */
 
#include <print.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <errno.h>
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
 * has (or hasn't) go according to having_go (typically false).
 * It also locks t->udebug.lock, making sure that t->udebug.debug_active
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
 * Returns EOK if all went well, or an error code otherwise.
 */
static int _thread_op_begin(thread_t *t, bool having_go)
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

	/* Verify that 't' is a userspace thread */
	if ((t->flags & THREAD_FLAG_USPACE) == 0) {
		/* It's not, deny its existence */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}

	/* Verify debugging state */
	if (t->udebug.debug_active != true) {
		/* Not in debugging session or undesired GO state */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}

	/*
	 * Since the thread has debug_active == true, TASK->udebug.lock
	 * is enough to ensure its existence and that debug_active remains
	 * true.
	 */
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);

	/* Only mutex TASK->udebug.lock left */
	
	/* Now verify that the thread belongs to the current task */
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

	/* The big task mutex is no longer needed */
	mutex_unlock(&TASK->udebug.lock);

	if (!t->udebug.stop != having_go) {
		/* Not in debugging session or undesired GO state */
		mutex_unlock(&t->udebug.lock);
		return EINVAL;
	}

	/* Only t->udebug.lock left */

	return EOK;	/* All went well */
}


static void _thread_op_end(thread_t *t)
{
	mutex_unlock(&t->udebug.lock);
}

/**
 * \return 0 (ok, but not done yet), 1 (done) or negative error code.
 */
int udebug_begin(call_t *call)
{
	int reply;

	thread_t *t;
	link_t *cur;

	printf("udebug_begin()\n");

	mutex_lock(&TASK->udebug.lock);
	printf("debugging task %llu\n", TASK->taskid);

	if (TASK->udebug.dt_state != UDEBUG_TS_INACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		printf("udebug_begin(): busy error\n");

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
	
	/* Set udebug.debug_active on all of the task's userspace threads */

	for (cur = TASK->th_head.next; cur != &TASK->th_head; cur = cur->next) {
		t = list_get_instance(cur, thread_t, th_link);

		mutex_lock(&t->udebug.lock);
		if ((t->flags & THREAD_FLAG_USPACE) != 0)
			t->udebug.debug_active = true;
		mutex_unlock(&t->udebug.lock);
	}

	mutex_unlock(&TASK->udebug.lock);

	printf("udebug_begin() done (%s)\n", 
	    reply ? "reply" : "stoppability wait");

	return reply;
}

int udebug_end(void)
{
	int rc;

	printf("udebug_end()\n");

	mutex_lock(&TASK->udebug.lock);
	printf("task %llu\n", TASK->taskid);

	rc = udebug_task_cleanup(TASK);

	mutex_unlock(&TASK->udebug.lock);

	return rc;
}

int udebug_set_evmask(udebug_evmask_t mask)
{
	printf("udebug_set_mask()\n");

	printf("debugging task %llu\n", TASK->taskid);

	mutex_lock(&TASK->udebug.lock);

	if (TASK->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		printf("udebug_set_mask(): not active debuging session\n");

		return EINVAL;
	}

	TASK->udebug.evmask = mask;

	mutex_unlock(&TASK->udebug.lock);

	return 0;
}


int udebug_go(thread_t *t, call_t *call)
{
	int rc;

//	printf("udebug_go()\n");

	/* On success, this will lock t->udebug.lock */
	rc = _thread_op_begin(t, false);
	if (rc != EOK) {
		return rc;
	}

	t->udebug.go_call = call;
	t->udebug.stop = false;
	t->udebug.cur_event = 0;	/* none */

	/*
	 * Neither t's lock nor threads_lock may be held during wakeup
	 */
	waitq_wakeup(&t->udebug.go_wq, WAKEUP_FIRST);

	_thread_op_end(t);

	return 0;
}

int udebug_stop(thread_t *t, call_t *call)
{
	int rc;

	printf("udebug_stop()\n");
	mutex_lock(&TASK->udebug.lock);

	/*
	 * On success, this will lock t->udebug.lock. Note that this makes sure
	 * the thread is not stopped.
	 */
	rc = _thread_op_begin(t, true);
	if (rc != EOK) {
		return rc;
	}

	/* Take GO away from the thread */
	t->udebug.stop = true;

	if (!t->udebug.stoppable) {
		/* Answer will be sent when the thread becomes stoppable */
		_thread_op_end(t);
		return 0;
	}

	/*
	 * Answer GO call
	 */
	printf("udebug_stop - answering go call\n");

	/* Make sure nobody takes this call away from us */
	call = t->udebug.go_call;
	t->udebug.go_call = NULL;

	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, UDEBUG_EVENT_STOP);
	printf("udebug_stop/ipc_answer\n");

	THREAD->udebug.cur_event = UDEBUG_EVENT_STOP;

	_thread_op_end(t);

	ipc_answer(&TASK->answerbox, call);
	mutex_unlock(&TASK->udebug.lock);

	printf("udebog_stop/done\n");
	return 0;
}

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

	printf("udebug_thread_read()\n");

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

		/* Not interested in kernel threads */
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

int udebug_args_read(thread_t *t, void **buffer)
{
	int rc;
	unative_t *arg_buffer;

//	printf("udebug_args_read()\n");

	/* Prepare a buffer to hold the arguments */
	arg_buffer = malloc(6 * sizeof(unative_t), 0);

	/* On success, this will lock t->udebug.lock */
	rc = _thread_op_begin(t, false);
	if (rc != EOK) {
		return rc;
	}

	/* Additionally we need to verify that we are inside a syscall */
	if (t->udebug.cur_event != UDEBUG_EVENT_SYSCALL_B &&
	    t->udebug.cur_event != UDEBUG_EVENT_SYSCALL_E) {
		_thread_op_end(t);
		return EINVAL;
	}

	/* Copy to a local buffer before releasing the lock */
	memcpy(arg_buffer, t->udebug.syscall_args, 6 * sizeof(unative_t));

	_thread_op_end(t);

	*buffer = arg_buffer;
	return 0;
}

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

//	printf("udebug_mem_read: src=%u, size=%u\n", uspace_addr, n);

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
