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
 * @brief Udebug hooks and data structure management.
 *
 * Udebug is an interface that makes userspace debuggers possible.
 */

#include <assert.h>
#include <debug.h>
#include <synch/waitq.h>
#include <udebug/udebug.h>
#include <errno.h>
#include <print.h>
#include <arch.h>
#include <proc/task.h>
#include <proc/thread.h>

/** Initialize udebug part of task structure.
 *
 * Called as part of task structure initialization.
 * @param ut Pointer to the structure to initialize.
 *
 */
void udebug_task_init(udebug_task_t *ut)
{
	mutex_initialize(&ut->lock, MUTEX_PASSIVE);
	ut->dt_state = UDEBUG_TS_INACTIVE;
	ut->begin_call = NULL;
	ut->not_stoppable_count = 0;
	ut->evmask = 0;
}

/** Initialize udebug part of thread structure.
 *
 * Called as part of thread structure initialization.
 *
 * @param ut Pointer to the structure to initialize.
 *
 */
void udebug_thread_initialize(udebug_thread_t *ut)
{
	mutex_initialize(&ut->lock, MUTEX_PASSIVE);
	waitq_initialize(&ut->go_wq);
	condvar_initialize(&ut->active_cv);

	ut->go_call = NULL;
	ut->uspace_state = NULL;
	ut->go = false;
	ut->stoppable = true;
	ut->active = false;
	ut->cur_event = 0;  /* None */
}

/** Wait for a GO message.
 *
 * When a debugging event occurs in a thread or the thread is stopped,
 * this function is called to block the thread until a GO message
 * is received.
 *
 * @param wq The wait queue used by the thread to wait for GO messages.
 *
 */
static void udebug_wait_for_go(waitq_t *wq)
{
	ipl_t ipl = waitq_sleep_prepare(wq);

	wq->missed_wakeups = 0;  /* Enforce blocking. */
	bool blocked;
	(void) waitq_sleep_timeout_unsafe(wq, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE, &blocked);
	waitq_sleep_finish(wq, blocked, ipl);
}

/** Start of stoppable section.
 *
 * A stoppable section is a section of code where if the thread can
 * be stoped. In other words, if a STOP operation is issued, the thread
 * is guaranteed not to execute any userspace instructions until the
 * thread is resumed.
 *
 * Having stoppable sections is better than having stopping points, since
 * a thread can be stopped even when it is blocked indefinitely in a system
 * call (whereas it would not reach any stopping point).
 *
 */
void udebug_stoppable_begin(void)
{
	assert(THREAD);
	assert(TASK);

	mutex_lock(&TASK->udebug.lock);

	int nsc = --TASK->udebug.not_stoppable_count;

	/* Lock order OK, THREAD->udebug.lock is after TASK->udebug.lock */
	mutex_lock(&THREAD->udebug.lock);
	assert(THREAD->udebug.stoppable == false);
	THREAD->udebug.stoppable = true;

	if ((TASK->udebug.dt_state == UDEBUG_TS_BEGINNING) && (nsc == 0)) {
		/*
		 * This was the last non-stoppable thread. Reply to
		 * DEBUG_BEGIN call.
		 *
		 */

		call_t *db_call = TASK->udebug.begin_call;
		assert(db_call);

		TASK->udebug.dt_state = UDEBUG_TS_ACTIVE;
		TASK->udebug.begin_call = NULL;

		IPC_SET_RETVAL(db_call->data, 0);
		ipc_answer(&TASK->answerbox, db_call);
	} else if (TASK->udebug.dt_state == UDEBUG_TS_ACTIVE) {
		/*
		 * Active debugging session
		 */

		if (THREAD->udebug.active == true &&
		    THREAD->udebug.go == false) {
			/*
			 * Thread was requested to stop - answer go call
			 *
			 */

			/* Make sure nobody takes this call away from us */
			call_t *go_call = THREAD->udebug.go_call;
			THREAD->udebug.go_call = NULL;
			assert(go_call);

			IPC_SET_RETVAL(go_call->data, 0);
			IPC_SET_ARG1(go_call->data, UDEBUG_EVENT_STOP);

			THREAD->udebug.cur_event = UDEBUG_EVENT_STOP;
			ipc_answer(&TASK->answerbox, go_call);
		}
	}

	mutex_unlock(&THREAD->udebug.lock);
	mutex_unlock(&TASK->udebug.lock);
}

/** End of a stoppable section.
 *
 * This is the point where the thread will block if it is stopped.
 * (As, by definition, a stopped thread must not leave its stoppable
 * section).
 *
 */
void udebug_stoppable_end(void)
{
restart:
	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	if ((THREAD->udebug.active) && (THREAD->udebug.go == false)) {
		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);

		udebug_wait_for_go(&THREAD->udebug.go_wq);

		goto restart;
		/* Must try again - have to lose stoppability atomically. */
	} else {
		++TASK->udebug.not_stoppable_count;
		assert(THREAD->udebug.stoppable == true);
		THREAD->udebug.stoppable = false;

		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
	}
}

/** Upon being scheduled to run, check if the current thread should stop.
 *
 * This function is called from clock().
 *
 */
void udebug_before_thread_runs(void)
{
	/* Check if we're supposed to stop */
	udebug_stoppable_begin();
	udebug_stoppable_end();
}

/** Syscall event hook.
 *
 * Must be called before and after servicing a system call. This generates
 * a SYSCALL_B or SYSCALL_E event, depending on the value of @a end_variant.
 *
 */
void udebug_syscall_event(sysarg_t a1, sysarg_t a2, sysarg_t a3,
    sysarg_t a4, sysarg_t a5, sysarg_t a6, sysarg_t id, sysarg_t rc,
    bool end_variant)
{
	udebug_event_t etype =
	    end_variant ? UDEBUG_EVENT_SYSCALL_E : UDEBUG_EVENT_SYSCALL_B;

	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	/* Must only generate events when in debugging session and is go. */
	if (THREAD->udebug.active != true || THREAD->udebug.go == false ||
	    (TASK->udebug.evmask & UDEBUG_EVMASK(etype)) == 0) {
		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
		return;
	}

	/* Fill in the GO response. */
	call_t *call = THREAD->udebug.go_call;
	THREAD->udebug.go_call = NULL;

	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, etype);
	IPC_SET_ARG2(call->data, id);
	IPC_SET_ARG3(call->data, rc);

	THREAD->udebug.syscall_args[0] = a1;
	THREAD->udebug.syscall_args[1] = a2;
	THREAD->udebug.syscall_args[2] = a3;
	THREAD->udebug.syscall_args[3] = a4;
	THREAD->udebug.syscall_args[4] = a5;
	THREAD->udebug.syscall_args[5] = a6;

	/*
	 * Make sure udebug.go is false when going to sleep
	 * in case we get woken up by DEBUG_END. (At which
	 * point it must be back to the initial true value).
	 *
	 */
	THREAD->udebug.go = false;
	THREAD->udebug.cur_event = etype;

	ipc_answer(&TASK->answerbox, call);

	mutex_unlock(&THREAD->udebug.lock);
	mutex_unlock(&TASK->udebug.lock);

	udebug_wait_for_go(&THREAD->udebug.go_wq);
}

/** Thread-creation event hook combined with attaching the thread.
 *
 * Must be called when a new userspace thread is created in the debugged
 * task. Generates a THREAD_B event. Also attaches the thread @a t
 * to the task @a ta.
 *
 * This is necessary to avoid a race condition where the BEGIN and THREAD_READ
 * requests would be handled inbetween attaching the thread and checking it
 * for being in a debugging session to send the THREAD_B event. We could then
 * either miss threads or get some threads both in the thread list
 * and get a THREAD_B event for them.
 *
 * @param thread Structure of the thread being created. Not locked, as the
 *               thread is not executing yet.
 * @param task   Task to which the thread should be attached.
 *
 */
void udebug_thread_b_event_attach(struct thread *thread, struct task *task)
{
	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	thread_attach(thread, task);

	LOG("Check state");

	/* Must only generate events when in debugging session */
	if (THREAD->udebug.active != true) {
		LOG("udebug.active: %s, udebug.go: %s",
		    THREAD->udebug.active ? "Yes(+)" : "No",
		    THREAD->udebug.go ? "Yes(-)" : "No");

		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
		return;
	}

	LOG("Trigger event");

	call_t *call = THREAD->udebug.go_call;

	THREAD->udebug.go_call = NULL;
	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, UDEBUG_EVENT_THREAD_B);
	IPC_SET_ARG2(call->data, (sysarg_t) thread);

	/*
	 * Make sure udebug.go is false when going to sleep
	 * in case we get woken up by DEBUG_END. (At which
	 * point it must be back to the initial true value).
	 *
	 */
	THREAD->udebug.go = false;
	THREAD->udebug.cur_event = UDEBUG_EVENT_THREAD_B;

	ipc_answer(&TASK->answerbox, call);

	mutex_unlock(&THREAD->udebug.lock);
	mutex_unlock(&TASK->udebug.lock);

	LOG("Wait for Go");
	udebug_wait_for_go(&THREAD->udebug.go_wq);
}

/** Thread-termination event hook.
 *
 * Must be called when the current thread is terminating.
 * Generates a THREAD_E event.
 *
 */
void udebug_thread_e_event(void)
{
	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	LOG("Check state");

	/* Must only generate events when in debugging session. */
	if (THREAD->udebug.active != true) {
		LOG("udebug.active: %s, udebug.go: %s",
		    THREAD->udebug.active ? "Yes" : "No",
		    THREAD->udebug.go ? "Yes" : "No");

		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
		return;
	}

	LOG("Trigger event");

	call_t *call = THREAD->udebug.go_call;

	THREAD->udebug.go_call = NULL;
	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, UDEBUG_EVENT_THREAD_E);

	/* Prevent any further debug activity in thread. */
	THREAD->udebug.active = false;
	THREAD->udebug.cur_event = 0;   /* None */
	THREAD->udebug.go = false;      /* Set to initial value */

	ipc_answer(&TASK->answerbox, call);

	mutex_unlock(&THREAD->udebug.lock);
	mutex_unlock(&TASK->udebug.lock);

	/*
	 * This event does not sleep - debugging has finished
	 * in this thread.
	 *
	 */
}

/** Terminate task debugging session.
 *
 * Gracefully terminate the debugging session for a task. If the debugger
 * is still waiting for events on some threads, it will receive a
 * FINISHED event for each of them.
 *
 * @param task Task structure. task->udebug.lock must be already locked.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t udebug_task_cleanup(struct task *task)
{
	assert(mutex_locked(&task->udebug.lock));

	if ((task->udebug.dt_state != UDEBUG_TS_BEGINNING) &&
	    (task->udebug.dt_state != UDEBUG_TS_ACTIVE)) {
		return EINVAL;
	}

	LOG("Task %" PRIu64, task->taskid);

	/* Finish debugging of all userspace threads */
	list_foreach(task->threads, th_link, thread_t, thread) {
		mutex_lock(&thread->udebug.lock);

		/* Only process userspace threads. */
		if (thread->uspace) {
			/* Prevent any further debug activity in thread. */
			thread->udebug.active = false;
			thread->udebug.cur_event = 0;   /* None */

			/* Is the thread still go? */
			if (thread->udebug.go == true) {
				/*
				 * Yes, so clear go. As active == false,
				 * this doesn't affect anything.
				 (
				 */
				thread->udebug.go = false;

				/* Answer GO call */
				LOG("Answer GO call with EVENT_FINISHED.");

				IPC_SET_RETVAL(thread->udebug.go_call->data, 0);
				IPC_SET_ARG1(thread->udebug.go_call->data,
				    UDEBUG_EVENT_FINISHED);

				ipc_answer(&task->answerbox, thread->udebug.go_call);
				thread->udebug.go_call = NULL;
			} else {
				/*
				 * Debug_stop is already at initial value.
				 * Yet this means the thread needs waking up.
				 *
				 */

				/*
				 * thread's lock must not be held when calling
				 * waitq_wakeup.
				 *
				 */
				waitq_wakeup(&thread->udebug.go_wq, WAKEUP_FIRST);
			}

			mutex_unlock(&thread->udebug.lock);
			condvar_broadcast(&thread->udebug.active_cv);
		} else
			mutex_unlock(&thread->udebug.lock);
	}

	task->udebug.dt_state = UDEBUG_TS_INACTIVE;
	task->udebug.debugger = NULL;

	return 0;
}

/** Wait for debugger to handle a fault in this thread.
 *
 * When a thread faults and someone is subscribed to the FAULT kernel event,
 * this function is called to wait for a debugging session to give userspace
 * a chance to examine the faulting thead/task. When the debugging session
 * is over, this function returns (so that thread/task cleanup can continue).
 *
 */
void udebug_thread_fault(void)
{
	udebug_stoppable_begin();

	/* Wait until a debugger attends to us. */
	mutex_lock(&THREAD->udebug.lock);
	while (!THREAD->udebug.active)
		condvar_wait(&THREAD->udebug.active_cv, &THREAD->udebug.lock);
	mutex_unlock(&THREAD->udebug.lock);

	/* Make sure the debugging session is over before proceeding. */
	mutex_lock(&THREAD->udebug.lock);
	while (THREAD->udebug.active)
		condvar_wait(&THREAD->udebug.active_cv, &THREAD->udebug.lock);
	mutex_unlock(&THREAD->udebug.lock);

	udebug_stoppable_end();
}

/** @}
 */
