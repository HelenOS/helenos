/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2007 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <libadt/list.h>
#include <psthread.h>
#include <malloc.h>
#include <unistd.h>
#include <thread.h>
#include <stdio.h>
#include <libarch/faddr.h>
#include <futex.h>
#include <assert.h>
#include <async.h>

#ifndef PSTHREAD_INITIAL_STACK_PAGES_NO
#define PSTHREAD_INITIAL_STACK_PAGES_NO	1
#endif

static LIST_INITIALIZE(ready_list);
static LIST_INITIALIZE(serialized_list);
static LIST_INITIALIZE(manager_list);

static void psthread_main(void);

static atomic_t psthread_futex = FUTEX_INITIALIZER;
/** Count of real threads that are in async_serialized mode */
static int serialized_threads;	/* Protected by async_futex */
/** Thread-local count of serialization. If >0, we must not preempt */
static __thread int serialization_count;
/** Counter of threads residing in async_manager */
static int threads_in_manager;

/** Setup psthread information into TCB structure */
psthread_data_t *psthread_setup(void)
{
	psthread_data_t *pt;
	tcb_t *tcb;

	tcb = __make_tls();
	if (!tcb)
		return NULL;

	pt = malloc(sizeof(*pt));
	if (!pt) {
		__free_tls(tcb);
		return NULL;
	}

	tcb->pst_data = pt;
	pt->tcb = tcb;

	return pt;
}

void psthread_teardown(psthread_data_t *pt)
{
	__free_tls(pt->tcb);
	free(pt);
}

/** Function that spans the whole life-cycle of a pseudo thread.
 *
 * Each pseudo thread begins execution in this function.
 * Then the function implementing the pseudo thread logic is called.
 * After its return, the return value is saved for a potentional
 * joiner. If the joiner exists, it is woken up. The pseudo thread
 * then switches to another pseudo thread, which cleans up after it.
 */
void psthread_main(void)
{
	psthread_data_t *pt = __tcb_get()->pst_data;

	pt->retval = pt->func(pt->arg);

	/*
	 * If there is a joiner, wake it up and save our return value.
	 */
	if (pt->joiner) {
		list_append(&pt->joiner->link, &ready_list);
		pt->joiner->joinee_retval = pt->retval;
	}

	psthread_schedule_next_adv(PS_FROM_DEAD);
	/* not reached */
}

/** Schedule next userspace pseudo thread.
 *
 * If calling with PS_TO_MANAGER parameter, the async_futex should be
 * held.
 *
 * @param ctype		One of PS_SLEEP, PS_PREEMPT, PS_TO_MANAGER,
 * 			PS_FROM_MANAGER, PS_FROM_DEAD. The parameter describes
 *			the circumstances of the switch.
 * @return		Return 0 if there is no ready pseudo thread,
 * 			return 1 otherwise.
 */
int psthread_schedule_next_adv(pschange_type ctype)
{
	psthread_data_t *srcpt, *dstpt;
	int retval = 0;
	
	futex_down(&psthread_futex);

	if (ctype == PS_PREEMPT && list_empty(&ready_list))
		goto ret_0;
	if (ctype == PS_SLEEP) {
		if (list_empty(&ready_list) && list_empty(&serialized_list))
			goto ret_0;
	}

	if (ctype == PS_FROM_MANAGER) {
		if (list_empty(&ready_list) && list_empty(&serialized_list))
			goto ret_0;
		/*
		 * Do not preempt if there is not sufficient count of thread
		 * managers.
		 */
		if (list_empty(&serialized_list) && threads_in_manager <=
		    serialized_threads) {
			goto ret_0;
		}
	}
	/* If we are going to manager and none exists, create it */
	if (ctype == PS_TO_MANAGER || ctype == PS_FROM_DEAD) {
		while (list_empty(&manager_list)) {
			futex_up(&psthread_futex);
			async_create_manager();
			futex_down(&psthread_futex);
		}
	}
	
	srcpt = __tcb_get()->pst_data;
	if (ctype != PS_FROM_DEAD) {
		/* Save current state */
		if (!context_save(&srcpt->ctx)) {
			if (serialization_count)
				srcpt->flags &= ~PSTHREAD_SERIALIZED;
			if (srcpt->clean_after_me) {
				/*
				 * Cleanup after the dead pseudo thread from
				 * which we restored context here.
				 */
				free(srcpt->clean_after_me->stack);
				psthread_teardown(srcpt->clean_after_me);
				srcpt->clean_after_me = NULL;
			}
			return 1;	/* futex_up already done here */
		}

		/* Save myself to the correct run list */
		if (ctype == PS_PREEMPT)
			list_append(&srcpt->link, &ready_list);
		else if (ctype == PS_FROM_MANAGER) {
			list_append(&srcpt->link, &manager_list);
			threads_in_manager--;
		} else {	
			/*
			 * If ctype == PS_TO_MANAGER, don't save ourselves to
			 * any list, we should already be somewhere, or we will
			 * be lost.
			 *
			 * The ctype == PS_SLEEP case is similar. The pseudo
			 * thread has an external refernce which can be used to
			 * wake it up once that time has come.
			 */
		}
	}

	/* Choose new thread to run */
	if (ctype == PS_TO_MANAGER || ctype == PS_FROM_DEAD) {
		dstpt = list_get_instance(manager_list.next, psthread_data_t,
		    link);
		if (serialization_count && ctype == PS_TO_MANAGER) {
			serialized_threads++;
			srcpt->flags |= PSTHREAD_SERIALIZED;
		}
		threads_in_manager++;

		if (ctype == PS_FROM_DEAD)
			dstpt->clean_after_me = srcpt;
	} else {
		if (!list_empty(&serialized_list)) {
			dstpt = list_get_instance(serialized_list.next,
			    psthread_data_t, link);
			serialized_threads--;
		} else {
			dstpt = list_get_instance(ready_list.next,
			    psthread_data_t, link);
		}
	}
	list_remove(&dstpt->link);

	futex_up(&psthread_futex);
	context_restore(&dstpt->ctx);
	/* not reached */

ret_0:
	futex_up(&psthread_futex);
	return retval;
}

/** Wait for uspace pseudo thread to finish.
 *
 * Each pseudo thread can be only joined by one other pseudo thread. Moreover,
 * the joiner must be from the same thread as the joinee.
 *
 * @param psthrid	Pseudo thread to join.
 *
 * @return		Value returned by the finished thread.
 */
int psthread_join(pstid_t psthrid)
{
	psthread_data_t *pt;
	psthread_data_t *cur;

	/* Handle psthrid = Kernel address -> it is wait for call */
	pt = (psthread_data_t *) psthrid;

	/*
	 * The joiner is running so the joinee isn't.
	 */
	cur = __tcb_get()->pst_data;
	pt->joiner = cur;
	psthread_schedule_next_adv(PS_SLEEP);

	/*
	 * The joinee fills in the return value.
	 */
	return cur->joinee_retval;
}

/** Create a userspace pseudo thread.
 *
 * @param func		Pseudo thread function.
 * @param arg		Argument to pass to func.
 *
 * @return		Return 0 on failure or TLS of the new pseudo thread.
 */
pstid_t psthread_create(int (*func)(void *), void *arg)
{
	psthread_data_t *pt;

	pt = psthread_setup();
	if (!pt) 
		return 0;
	pt->stack = (char *) malloc(PSTHREAD_INITIAL_STACK_PAGES_NO *
	    getpagesize());

	if (!pt->stack) {
		psthread_teardown(pt);
		return 0;
	}

	pt->arg= arg;
	pt->func = func;
	pt->clean_after_me = NULL;
	pt->joiner = NULL;
	pt->joinee_retval = 0;
	pt->retval = 0;
	pt->flags = 0;

	context_save(&pt->ctx);
	context_set(&pt->ctx, FADDR(psthread_main), pt->stack,
	    PSTHREAD_INITIAL_STACK_PAGES_NO * getpagesize(), pt->tcb);

	return (pstid_t) pt;
}

/** Add a thread to the ready list.
 *
 * @param psthrid		Pinter to the pseudo thread structure of the
 *				pseudo thread to be added.
 */
void psthread_add_ready(pstid_t psthrid)
{
	psthread_data_t *pt;

	pt = (psthread_data_t *) psthrid;
	futex_down(&psthread_futex);
	if ((pt->flags & PSTHREAD_SERIALIZED))
		list_append(&pt->link, &serialized_list);
	else
		list_append(&pt->link, &ready_list);
	futex_up(&psthread_futex);
}

/** Add a pseudo thread to the manager list.
 *
 * @param psthrid		Pinter to the pseudo thread structure of the
 *				pseudo thread to be added.
 */
void psthread_add_manager(pstid_t psthrid)
{
	psthread_data_t *pt;

	pt = (psthread_data_t *) psthrid;

	futex_down(&psthread_futex);
	list_append(&pt->link, &manager_list);
	futex_up(&psthread_futex);
}

/** Remove one manager from manager list */
void psthread_remove_manager(void)
{
	futex_down(&psthread_futex);
	if (list_empty(&manager_list)) {
		futex_up(&psthread_futex);
		return;
	}
	list_remove(manager_list.next);
	futex_up(&psthread_futex);
}

/** Return thread id of the currently running thread.
 *
 * @return		Pseudo thread ID of the currently running pseudo thread.
 */
pstid_t psthread_get_id(void)
{
	return (pstid_t) __tcb_get()->pst_data;
}

/** Disable preemption 
 *
 * If the thread wants to send several message in a row and does not want to be
 * preempted, it should start async_serialize_start() in the beginning of
 * communication and async_serialize_end() in the end. If it is a true
 * multithreaded application, it should protect the communication channel by a
 * futex as well. Interrupt messages can still be preempted.
 */
void psthread_inc_sercount(void)
{
	serialization_count++;
}

/** Restore the preemption counter to the previous state. */
void psthread_dec_sercount(void)
{
	serialization_count--;
}

/** @}
 */

