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
 * @brief Udebug operations.
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
#include <stdbool.h>
#include <str.h>
#include <syscall/copy.h>
#include <ipc/ipc.h>
#include <udebug/udebug.h>
#include <udebug/udebug_ops.h>
#include <mem.h>

/** Prepare a thread for a debugging operation.
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
 * @param thread   Pointer, need not at all be valid.
 * @param being_go Required thread state.
 *
 * Returns EOK if all went well, or an error code otherwise.
 *
 */
static errno_t _thread_op_begin(thread_t *thread, bool being_go)
{
	mutex_lock(&TASK->udebug.lock);
	
	/* thread_exists() must be called with threads_lock held */
	irq_spinlock_lock(&threads_lock, true);
	
	if (!thread_exists(thread)) {
		irq_spinlock_unlock(&threads_lock, true);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}
	
	/* thread->lock is enough to ensure the thread's existence */
	irq_spinlock_exchange(&threads_lock, &thread->lock);
	
	/* Verify that 'thread' is a userspace thread. */
	if (!thread->uspace) {
		/* It's not, deny its existence */
		irq_spinlock_unlock(&thread->lock, true);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}
	
	/* Verify debugging state. */
	if (thread->udebug.active != true) {
		/* Not in debugging session or undesired GO state */
		irq_spinlock_unlock(&thread->lock, true);
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}
	
	/*
	 * Since the thread has active == true, TASK->udebug.lock
	 * is enough to ensure its existence and that active remains
	 * true.
	 *
	 */
	irq_spinlock_unlock(&thread->lock, true);
	
	/* Only mutex TASK->udebug.lock left. */
	
	/* Now verify that the thread belongs to the current task. */
	if (thread->task != TASK) {
		/* No such thread belonging this task*/
		mutex_unlock(&TASK->udebug.lock);
		return ENOENT;
	}
	
	/*
	 * Now we need to grab the thread's debug lock for synchronization
	 * of the threads stoppability/stop state.
	 *
	 */
	mutex_lock(&thread->udebug.lock);
	
	/* The big task mutex is no longer needed. */
	mutex_unlock(&TASK->udebug.lock);
	
	if (thread->udebug.go != being_go) {
		/* Not in debugging session or undesired GO state. */
		mutex_unlock(&thread->udebug.lock);
		return EINVAL;
	}
	
	/* Only thread->udebug.lock left. */
	
	return EOK;  /* All went well. */
}

/** End debugging operation on a thread. */
static void _thread_op_end(thread_t *thread)
{
	mutex_unlock(&thread->udebug.lock);
}

/** Begin debugging the current task.
 *
 * Initiates a debugging session for the current task (and its threads).
 * When the debugging session has started a reply should be sent to the
 * UDEBUG_BEGIN call. This may happen immediately in this function if
 * all the threads in this task are stoppable at the moment and in this
 * case the function sets @a *active to @c true.
 *
 * Otherwise the function sets @a *active to false and the resonse should
 * be sent as soon as all the threads become stoppable (i.e. they can be
 * considered stopped).
 *
 * @param call The BEGIN call we are servicing.
 * @param active Place to store @c true iff we went directly to active state,
 *               @c false if we only went to beginning state
 *
 * @return EOK on success, EBUSY if the task is already has an active
 *         debugging session.
 */
errno_t udebug_begin(call_t *call, bool *active)
{
	LOG("Debugging task %" PRIu64, TASK->taskid);
	
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
		*active = true;  /* directly to active state */
	} else
		*active = false;  /* only in beginning state */
	
	/* Set udebug.active on all of the task's userspace threads. */
	
	list_foreach(TASK->threads, th_link, thread_t, thread) {
		mutex_lock(&thread->udebug.lock);
		if (thread->uspace) {
			thread->udebug.active = true;
			mutex_unlock(&thread->udebug.lock);
			condvar_broadcast(&thread->udebug.active_cv);
		} else
			mutex_unlock(&thread->udebug.lock);
	}
	
	mutex_unlock(&TASK->udebug.lock);
	return EOK;
}

/** Finish debugging the current task.
 *
 * Closes the debugging session for the current task.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t udebug_end(void)
{
	LOG("Task %" PRIu64, TASK->taskid);
	
	mutex_lock(&TASK->udebug.lock);
	errno_t rc = udebug_task_cleanup(TASK);
	mutex_unlock(&TASK->udebug.lock);
	
	return rc;
}

/** Set the event mask.
 *
 * Sets the event mask that determines which events are enabled.
 *
 * @param mask Or combination of events that should be enabled.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t udebug_set_evmask(udebug_evmask_t mask)
{
	LOG("mask = 0x%x", mask);
	
	mutex_lock(&TASK->udebug.lock);
	
	if (TASK->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		return EINVAL;
	}
	
	TASK->udebug.evmask = mask;
	mutex_unlock(&TASK->udebug.lock);
	
	return EOK;
}

/** Give thread GO.
 *
 * Upon recieving a go message, the thread is given GO. Being GO
 * means the thread is allowed to execute userspace code (until
 * a debugging event or STOP occurs, at which point the thread loses GO.
 *
 * @param thread The thread to operate on (unlocked and need not be valid).
 * @param call   The GO call that we are servicing.
 *
 */
errno_t udebug_go(thread_t *thread, call_t *call)
{
	/* On success, this will lock thread->udebug.lock. */
	errno_t rc = _thread_op_begin(thread, false);
	if (rc != EOK)
		return rc;
	
	thread->udebug.go_call = call;
	thread->udebug.go = true;
	thread->udebug.cur_event = 0;  /* none */
	
	/*
	 * Neither thread's lock nor threads_lock may be held during wakeup.
	 *
	 */
	waitq_wakeup(&thread->udebug.go_wq, WAKEUP_FIRST);
	
	_thread_op_end(thread);
	
	return EOK;
}

/** Stop a thread (i.e. take its GO away)
 *
 * Generates a STOP event as soon as the thread becomes stoppable (i.e.
 * can be considered stopped).
 *
 * @param thread The thread to operate on (unlocked and need not be valid).
 * @param call   The GO call that we are servicing.
 *
 */
errno_t udebug_stop(thread_t *thread, call_t *call)
{
	LOG("udebug_stop()");
	
	/*
	 * On success, this will lock thread->udebug.lock. Note that this
	 * makes sure the thread is not stopped.
	 *
	 */
	errno_t rc = _thread_op_begin(thread, true);
	if (rc != EOK)
		return rc;
	
	/* Take GO away from the thread. */
	thread->udebug.go = false;
	
	if (thread->udebug.stoppable != true) {
		/* Answer will be sent when the thread becomes stoppable. */
		_thread_op_end(thread);
		return EOK;
	}
	
	/*
	 * Answer GO call.
	 *
	 */
	
	/* Make sure nobody takes this call away from us. */
	call = thread->udebug.go_call;
	thread->udebug.go_call = NULL;
	
	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, UDEBUG_EVENT_STOP);
	
	THREAD->udebug.cur_event = UDEBUG_EVENT_STOP;
	
	_thread_op_end(thread);
	
	mutex_lock(&TASK->udebug.lock);
	ipc_answer(&TASK->answerbox, call);
	mutex_unlock(&TASK->udebug.lock);
	
	return EOK;
}

/** Read the list of userspace threads in the current task.
 *
 * The list takes the form of a sequence of thread hashes (i.e. the pointers
 * to thread structures). A buffer of size @a buf_size is allocated and
 * a pointer to it written to @a buffer. The sequence of hashes is written
 * into this buffer.
 *
 * If the sequence is longer than @a buf_size bytes, only as much hashes
 * as can fit are copied. The number of bytes copied is stored in @a stored.
 * The total number of thread bytes that could have been saved had there been
 * enough space is stored in @a needed.
 *
 * The rationale for having @a buf_size is that this function is only
 * used for servicing the THREAD_READ message, which always specifies
 * a maximum size for the userspace buffer.
 *
 * @param buffer   The buffer for storing thread hashes.
 * @param buf_size Buffer size in bytes.
 * @param stored   The actual number of bytes copied will be stored here.
 * @param needed   Total number of hashes that could have been saved.
 *
 */
errno_t udebug_thread_read(void **buffer, size_t buf_size, size_t *stored,
    size_t *needed)
{
	LOG("udebug_thread_read()");
	
	/* Allocate a buffer to hold thread IDs */
	sysarg_t *id_buffer = malloc(buf_size + 1, 0);
	
	mutex_lock(&TASK->udebug.lock);
	
	/* Verify task state */
	if (TASK->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		free(id_buffer);
		return EINVAL;
	}
	
	irq_spinlock_lock(&TASK->lock, true);
	
	/* Copy down the thread IDs */
	
	size_t max_ids = buf_size / sizeof(sysarg_t);
	size_t copied_ids = 0;
	size_t extra_ids = 0;
	
	/* FIXME: make sure the thread isn't past debug shutdown... */
	list_foreach(TASK->threads, th_link, thread_t, thread) {
		irq_spinlock_lock(&thread->lock, false);
		bool uspace = thread->uspace;
		irq_spinlock_unlock(&thread->lock, false);
		
		/* Not interested in kernel threads. */
		if (!uspace)
			continue;
		
		if (copied_ids < max_ids) {
			/* Using thread struct pointer as identification hash */
			id_buffer[copied_ids++] = (sysarg_t) thread;
		} else
			extra_ids++;
	}
	
	irq_spinlock_unlock(&TASK->lock, true);
	
	mutex_unlock(&TASK->udebug.lock);
	
	*buffer = id_buffer;
	*stored = copied_ids * sizeof(sysarg_t);
	*needed = (copied_ids + extra_ids) * sizeof(sysarg_t);
	
	return EOK;
}

/** Read task name.
 *
 * Returns task name as non-terminated string in a newly allocated buffer.
 * Also returns the size of the data.
 *
 * @param data      Place to store pointer to newly allocated block.
 * @param data_size Place to store size of the data.
 *
 * @return EOK.
 *
 */
errno_t udebug_name_read(char **data, size_t *data_size)
{
	size_t name_size = str_size(TASK->name) + 1;
	
	*data = malloc(name_size, 0);
	*data_size = name_size;
	
	memcpy(*data, TASK->name, name_size);
	
	return EOK;
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
 * @param thread Thread where call arguments are to be read.
 * @param buffer Place to store pointer to new buffer.
 *
 * @return EOK on success, ENOENT if @a t is invalid, EINVAL
 *         if thread state is not valid for this operation.
 *
 */
errno_t udebug_args_read(thread_t *thread, void **buffer)
{
	/* On success, this will lock t->udebug.lock. */
	errno_t rc = _thread_op_begin(thread, false);
	if (rc != EOK)
		return rc;
	
	/* Additionally we need to verify that we are inside a syscall. */
	if ((thread->udebug.cur_event != UDEBUG_EVENT_SYSCALL_B) &&
	    (thread->udebug.cur_event != UDEBUG_EVENT_SYSCALL_E)) {
		_thread_op_end(thread);
		return EINVAL;
	}
	
	/* Prepare a buffer to hold the arguments. */
	sysarg_t *arg_buffer = malloc(6 * sizeof(sysarg_t), 0);
	
	/* Copy to a local buffer before releasing the lock. */
	memcpy(arg_buffer, thread->udebug.syscall_args, 6 * sizeof(sysarg_t));
	
	_thread_op_end(thread);
	
	*buffer = arg_buffer;
	return EOK;
}

/** Read the register state of the thread.
 *
 * The contents of the thread's istate structure are copied to a newly
 * allocated buffer and a pointer to it is written to @a buffer. The size of
 * the buffer will be sizeof(istate_t).
 *
 * Currently register state cannot be read if the thread is inside a system
 * call (as opposed to an exception). This is an implementation limit.
 *
 * @param thread Thread whose state is to be read.
 * @param buffer Place to store pointer to new buffer.
 *
 * @return EOK on success, ENOENT if @a t is invalid, EINVAL
 *         if thread is not in valid state, EBUSY if istate
 *         is not available.
 *
 */
errno_t udebug_regs_read(thread_t *thread, void **buffer)
{
	/* On success, this will lock t->udebug.lock */
	errno_t rc = _thread_op_begin(thread, false);
	if (rc != EOK)
		return rc;
	
	istate_t *state = thread->udebug.uspace_state;
	if (state == NULL) {
		_thread_op_end(thread);
		return EBUSY;
	}
	
	/* Prepare a buffer to hold the data. */
	istate_t *state_buf = malloc(sizeof(istate_t), 0);
	
	/* Copy to the allocated buffer */
	memcpy(state_buf, state, sizeof(istate_t));
	
	_thread_op_end(thread);
	
	*buffer = (void *) state_buf;
	return EOK;
}

/** Read the memory of the debugged task.
 *
 * Reads @a n bytes from the address space of the debugged task, starting
 * from @a uspace_addr. The bytes are copied into an allocated buffer
 * and a pointer to it is written into @a buffer.
 *
 * @param uspace_addr Address from where to start reading.
 * @param n           Number of bytes to read.
 * @param buffer      For storing a pointer to the allocated buffer.
 *
 */
errno_t udebug_mem_read(sysarg_t uspace_addr, size_t n, void **buffer)
{
	/* Verify task state */
	mutex_lock(&TASK->udebug.lock);
	
	if (TASK->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		mutex_unlock(&TASK->udebug.lock);
		return EBUSY;
	}
	
	void *data_buffer = malloc(n, 0);
	
	/*
	 * NOTE: this is not strictly from a syscall... but that shouldn't
	 * be a problem
	 *
	 */
	errno_t rc = copy_from_uspace(data_buffer, (void *) uspace_addr, n);
	mutex_unlock(&TASK->udebug.lock);
	
	if (rc != EOK)
		return rc;
	
	*buffer = data_buffer;
	return EOK;
}

/** @}
 */
