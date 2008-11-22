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
 * @brief	Udebug hooks and data structure management.
 *
 * Udebug is an interface that makes userspace debuggers possible.
 */
 
#include <synch/waitq.h>
#include <debug.h>
#include <udebug/udebug.h>
#include <errno.h>
#include <arch.h>


/** Initialize udebug part of task structure.
 *
 * Called as part of task structure initialization.
 * @param ut	Pointer to the structure to initialize.
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
 * @param ut	Pointer to the structure to initialize.
 */
void udebug_thread_initialize(udebug_thread_t *ut)
{
	mutex_initialize(&ut->lock, MUTEX_PASSIVE);
	waitq_initialize(&ut->go_wq);

	ut->go_call = NULL;
	ut->uspace_state = NULL;
	ut->go = false;
	ut->stoppable = true;
	ut->debug_active = false;
	ut->cur_event = 0; /* none */
}

/** Wait for a GO message.
 *
 * When a debugging event occurs in a thread or the thread is stopped,
 * this function is called to block the thread until a GO message
 * is received.
 *
 * @param wq	The wait queue used by the thread to wait for GO messages.
 */
static void udebug_wait_for_go(waitq_t *wq)
{
	int rc;
	ipl_t ipl;

	ipl = waitq_sleep_prepare(wq);

	wq->missed_wakeups = 0;	/* Enforce blocking. */
	rc = waitq_sleep_timeout_unsafe(wq, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);

	waitq_sleep_finish(wq, rc, ipl);
}

/** Do a preliminary check that a debugging session is in progress.
 * 
 * This only requires the THREAD->udebug.lock mutex (and not TASK->udebug.lock
 * mutex). For an undebugged task, this will never block (while there could be
 * collisions by different threads on the TASK mutex), thus improving SMP
 * perormance for undebugged tasks.
 *
 * @return	True if the thread was in a debugging session when the function
 *		checked, false otherwise.
 */
static bool udebug_thread_precheck(void)
{
	bool res;

	mutex_lock(&THREAD->udebug.lock);
	res = THREAD->udebug.debug_active;
	mutex_unlock(&THREAD->udebug.lock);

	return res;
}

/** Start of stoppable section.
 *
 * A stoppable section is a section of code where if the thread can be stoped. In other words,
 * if a STOP operation is issued, the thread is guaranteed not to execute
 * any userspace instructions until the thread is resumed.
 *
 * Having stoppable sections is better than having stopping points, since
 * a thread can be stopped even when it is blocked indefinitely in a system
 * call (whereas it would not reach any stopping point).
 */
void udebug_stoppable_begin(void)
{
	int nsc;
	call_t *db_call, *go_call;

	ASSERT(THREAD);
	ASSERT(TASK);

	/* Early check for undebugged tasks */
	if (!udebug_thread_precheck()) {
		return;
	}

	mutex_lock(&TASK->udebug.lock);

	nsc = --TASK->udebug.not_stoppable_count;

	/* Lock order OK, THREAD->udebug.lock is after TASK->udebug.lock */
	mutex_lock(&THREAD->udebug.lock);
	ASSERT(THREAD->udebug.stoppable == false);
	THREAD->udebug.stoppable = true;

	if (TASK->udebug.dt_state == UDEBUG_TS_BEGINNING && nsc == 0) {
		/*
		 * This was the last non-stoppable thread. Reply to
		 * DEBUG_BEGIN call.
		 */

		db_call = TASK->udebug.begin_call;
		ASSERT(db_call);

		TASK->udebug.dt_state = UDEBUG_TS_ACTIVE;
		TASK->udebug.begin_call = NULL;

		IPC_SET_RETVAL(db_call->data, 0);
		ipc_answer(&TASK->answerbox, db_call);		

	} else if (TASK->udebug.dt_state == UDEBUG_TS_ACTIVE) {
		/*
		 * Active debugging session
		 */

		if (THREAD->udebug.debug_active == true &&
		    THREAD->udebug.go == false) {
			/*
			 * Thread was requested to stop - answer go call
			 */

			/* Make sure nobody takes this call away from us */
			go_call = THREAD->udebug.go_call;
			THREAD->udebug.go_call = NULL;
			ASSERT(go_call);

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
 * (As, by definition, a stopped thread must not leave its stoppable section).
 */
void udebug_stoppable_end(void)
{
	/* Early check for undebugged tasks */
	if (!udebug_thread_precheck()) {
		return;
	}

restart:
	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	if (THREAD->udebug.debug_active &&
	    THREAD->udebug.go == false) {
		TASK->udebug.begin_call = NULL;
		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);

		udebug_wait_for_go(&THREAD->udebug.go_wq);

		goto restart;
		/* Must try again - have to lose stoppability atomically. */
	} else {
		++TASK->udebug.not_stoppable_count;
		ASSERT(THREAD->udebug.stoppable == true);
		THREAD->udebug.stoppable = false;

		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
	}
}

/** Upon being scheduled to run, check if the current thread should stop.
 *
 * This function is called from clock().
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
 */
void udebug_syscall_event(unative_t a1, unative_t a2, unative_t a3,
    unative_t a4, unative_t a5, unative_t a6, unative_t id, unative_t rc,
    bool end_variant)
{
	call_t *call;
	udebug_event_t etype;

	etype = end_variant ? UDEBUG_EVENT_SYSCALL_E : UDEBUG_EVENT_SYSCALL_B;

	/* Early check for undebugged tasks */
	if (!udebug_thread_precheck()) {
		return;
	}

	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	/* Must only generate events when in debugging session and is go. */
	if (THREAD->udebug.debug_active != true ||
	    THREAD->udebug.go == false ||
	    (TASK->udebug.evmask & UDEBUG_EVMASK(etype)) == 0) {
		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
		return;
	}

	//printf("udebug_syscall_event\n");
	call = THREAD->udebug.go_call;
	THREAD->udebug.go_call = NULL;

	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, etype);
	IPC_SET_ARG2(call->data, id);
	IPC_SET_ARG3(call->data, rc);
	//printf("udebug_syscall_event/ipc_answer\n");

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
	 */
	THREAD->udebug.go = false;
	THREAD->udebug.cur_event = etype;

	ipc_answer(&TASK->answerbox, call);

	mutex_unlock(&THREAD->udebug.lock);
	mutex_unlock(&TASK->udebug.lock);

	udebug_wait_for_go(&THREAD->udebug.go_wq);
}

/** Thread-creation event hook.
 *
 * Must be called when a new userspace thread is created in the debugged
 * task. Generates a THREAD_B event.
 *
 * @param t	Structure of the thread being created. Not locked, as the
 *		thread is not executing yet.
 */
void udebug_thread_b_event(struct thread *t)
{
	call_t *call;

	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	LOG("udebug_thread_b_event\n");
	LOG("- check state\n");

	/* Must only generate events when in debugging session */
	if (THREAD->udebug.debug_active != true) {
		LOG("- debug_active: %s, udebug.go: %s\n",
			THREAD->udebug.debug_active ? "yes(+)" : "no(-)",
			THREAD->udebug.go ? "yes(-)" : "no(+)");
		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
		return;
	}

	LOG("- trigger event\n");

	call = THREAD->udebug.go_call;
	THREAD->udebug.go_call = NULL;
	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, UDEBUG_EVENT_THREAD_B);
	IPC_SET_ARG2(call->data, (unative_t)t);

	/*
	 * Make sure udebug.go is false when going to sleep
	 * in case we get woken up by DEBUG_END. (At which
	 * point it must be back to the initial true value).
	 */
	THREAD->udebug.go = false;
	THREAD->udebug.cur_event = UDEBUG_EVENT_THREAD_B;

	ipc_answer(&TASK->answerbox, call);

	mutex_unlock(&THREAD->udebug.lock);
	mutex_unlock(&TASK->udebug.lock);

	LOG("- sleep\n");
	udebug_wait_for_go(&THREAD->udebug.go_wq);
}

/** Thread-termination event hook.
 *
 * Must be called when the current thread is terminating.
 * Generates a THREAD_E event.
 */
void udebug_thread_e_event(void)
{
	call_t *call;

	mutex_lock(&TASK->udebug.lock);
	mutex_lock(&THREAD->udebug.lock);

	LOG("udebug_thread_e_event\n");
	LOG("- check state\n");

	/* Must only generate events when in debugging session. */
	if (THREAD->udebug.debug_active != true) {
/*		printf("- debug_active: %s, udebug.go: %s\n",
			THREAD->udebug.debug_active ? "yes(+)" : "no(-)",
			THREAD->udebug.go ? "yes(-)" : "no(+)");*/
		mutex_unlock(&THREAD->udebug.lock);
		mutex_unlock(&TASK->udebug.lock);
		return;
	}

	LOG("- trigger event\n");

	call = THREAD->udebug.go_call;
	THREAD->udebug.go_call = NULL;
	IPC_SET_RETVAL(call->data, 0);
	IPC_SET_ARG1(call->data, UDEBUG_EVENT_THREAD_E);

	/* Prevent any further debug activity in thread. */
	THREAD->udebug.debug_active = false;
	THREAD->udebug.cur_event = 0;		/* none */
	THREAD->udebug.go = false;	/* set to initial value */

	ipc_answer(&TASK->answerbox, call);

	mutex_unlock(&THREAD->udebug.lock);
	mutex_unlock(&TASK->udebug.lock);

	/* 
	 * This event does not sleep - debugging has finished
	 * in this thread.
	 */
}

/**
 * Terminate task debugging session.
 *
 * Gracefully terminates the debugging session for a task. If the debugger
 * is still waiting for events on some threads, it will receive a
 * FINISHED event for each of them.
 *
 * @param ta	Task structure. ta->udebug.lock must be already locked.
 * @return	Zero on success or negative error code.
 */
int udebug_task_cleanup(struct task *ta)
{
	thread_t *t;
	link_t *cur;
	int flags;
	ipl_t ipl;

	LOG("udebug_task_cleanup()\n");
	LOG("task %" PRIu64 "\n", ta->taskid);

	if (ta->udebug.dt_state != UDEBUG_TS_BEGINNING &&
	    ta->udebug.dt_state != UDEBUG_TS_ACTIVE) {
		LOG("udebug_task_cleanup(): task not being debugged\n");
		return EINVAL;
	}

	/* Finish debugging of all userspace threads */
	for (cur = ta->th_head.next; cur != &ta->th_head; cur = cur->next) {
		t = list_get_instance(cur, thread_t, th_link);

		mutex_lock(&t->udebug.lock);

		ipl = interrupts_disable();
		spinlock_lock(&t->lock);

		flags = t->flags;

		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);

		/* Only process userspace threads. */
		if ((flags & THREAD_FLAG_USPACE) != 0) {
			/* Prevent any further debug activity in thread. */
			t->udebug.debug_active = false;
			t->udebug.cur_event = 0;	/* none */

			/* Is the thread still go? */
			if (t->udebug.go == true) {
				/*
				* Yes, so clear go. As debug_active == false,
				 * this doesn't affect anything.
				 */
				t->udebug.go = false;	

				/* Answer GO call */
				LOG("answer GO call with EVENT_FINISHED\n");
				IPC_SET_RETVAL(t->udebug.go_call->data, 0);
				IPC_SET_ARG1(t->udebug.go_call->data,
				    UDEBUG_EVENT_FINISHED);

				ipc_answer(&ta->answerbox, t->udebug.go_call);
				t->udebug.go_call = NULL;
			} else {
				/*
				 * Debug_stop is already at initial value.
				 * Yet this means the thread needs waking up.
				 */

				/*
				 * t's lock must not be held when calling
				 * waitq_wakeup.
				 */
				waitq_wakeup(&t->udebug.go_wq, WAKEUP_FIRST);
			}
		}
		mutex_unlock(&t->udebug.lock);
	}

	ta->udebug.dt_state = UDEBUG_TS_INACTIVE;
	ta->udebug.debugger = NULL;

	return 0;
}


/** @}
 */
