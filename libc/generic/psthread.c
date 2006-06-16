/*
 * Copyright (C) 2006 Ondrej Palkovsky
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
#include <kernel/arch/faddr.h>
#include <futex.h>
#include <assert.h>
#include <async.h>

#ifndef PSTHREAD_INITIAL_STACK_PAGES_NO
#define PSTHREAD_INITIAL_STACK_PAGES_NO 1
#endif

static LIST_INITIALIZE(ready_list);
static LIST_INITIALIZE(serialized_list);
static LIST_INITIALIZE(manager_list);

static void psthread_main(void);

static atomic_t psthread_futex = FUTEX_INITIALIZER;
/** Count of real threads that are in async_serialized mode */
static int serialized_threads; /* Protected by async_futex */
/** Thread-local count of serialization. If >0, we must not preempt */
static __thread int serialization_count;
/** Counter of threads residing in async_manager */
static int threads_in_manager;

/** Setup PSthread information into TCB structure */
psthread_data_t * psthread_setup()
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

/** Function that is called on entry to new uspace thread */
void psthread_main(void)
{
	psthread_data_t *pt = __tcb_get()->pst_data;

	pt->retval = pt->func(pt->arg);

	pt->finished = 1;
	if (pt->waiter)
		list_append(&pt->waiter->link, &ready_list);

	psthread_schedule_next_adv(PS_FROM_DEAD);
}

/** Schedule next userspace pseudo thread.
 *
 * If calling with PS_TO_MANAGER parameter, the async_futex should be
 * held.
 *
 * @param ctype Type of switch.
 * @return 0 if there is no ready pseudo thread, 1 otherwise.
 */
int psthread_schedule_next_adv(pschange_type ctype)
{
	psthread_data_t *srcpt, *dstpt;
	int retval = 0;
	
	futex_down(&psthread_futex);

	if (ctype == PS_PREEMPT && list_empty(&ready_list))
		goto ret_0;

	if (ctype == PS_FROM_MANAGER) {
		if (list_empty(&ready_list) && list_empty(&serialized_list))
			goto ret_0;
		/* Do not preempt if there is not sufficient count of thread managers */
		if (list_empty(&serialized_list) && threads_in_manager <= serialized_threads) {
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
	
	if (ctype != PS_FROM_DEAD) {
		/* Save current state */
		srcpt = __tcb_get()->pst_data;
		if (!context_save(&srcpt->ctx)) {
			if (serialization_count)
				srcpt->flags &= ~PSTHREAD_SERIALIZED;
			return 1; // futex_up already done here
		}

		/* Save myself to correct run list */
		if (ctype == PS_PREEMPT)
			list_append(&srcpt->link, &ready_list);
		else if (ctype == PS_FROM_MANAGER) {
			list_append(&srcpt->link, &manager_list);
			threads_in_manager--;
		} /* If ctype == PS_TO_MANAGER, don't save ourselves to any list, we should
		   * already be somewhere, or we will be lost */
	}

	/* Choose new thread to run */
	if (ctype == PS_TO_MANAGER || ctype == PS_FROM_DEAD) {
		dstpt = list_get_instance(manager_list.next,psthread_data_t, link);
		if (serialization_count && ctype == PS_TO_MANAGER) {
			serialized_threads++;
			srcpt->flags |= PSTHREAD_SERIALIZED;
		}
		threads_in_manager++;
	} else {
		if (!list_empty(&serialized_list)) {
			dstpt = list_get_instance(serialized_list.next, psthread_data_t, link);
			serialized_threads--;
		} else
			dstpt = list_get_instance(ready_list.next, psthread_data_t, link);
	}
	list_remove(&dstpt->link);

	futex_up(&psthread_futex);
	context_restore(&dstpt->ctx);

ret_0:
	futex_up(&psthread_futex);
	return retval;
}

/** Wait for uspace pseudo thread to finish.
 *
 * @param psthrid Pseudo thread to wait for.
 *
 * @return Value returned by the finished thread.
 */
int psthread_join(pstid_t psthrid)
{
	volatile psthread_data_t *pt;
	volatile int retval;

	/* Handle psthrid = Kernel address -> it is wait for call */
	pt = (psthread_data_t *) psthrid;

	/* TODO */
	printf("join unsupported\n");
	_exit(1);

	retval = pt->retval;

	free(pt->stack);
	psthread_teardown((void *)pt);

	return retval;
}

/**
 * Create a userspace thread
 *
 * @param func Pseudo thread function.
 * @param arg Argument to pass to func.
 *
 * @return 0 on failure, TLS of the new pseudo thread.
 */
pstid_t psthread_create(int (*func)(void *), void *arg)
{
	psthread_data_t *pt;

	pt = psthread_setup();
	if (!pt) 
		return 0;
	pt->stack = (char *) malloc(PSTHREAD_INITIAL_STACK_PAGES_NO*getpagesize());

	if (!pt->stack) {
		psthread_teardown(pt);
		return 0;
	}

	pt->arg= arg;
	pt->func = func;
	pt->finished = 0;
	pt->waiter = NULL;
	pt->flags = 0;

	context_save(&pt->ctx);
	context_set(&pt->ctx, FADDR(psthread_main), pt->stack, PSTHREAD_INITIAL_STACK_PAGES_NO*getpagesize(), pt->tcb);

	return (pstid_t )pt;
}

/** Add a thread to ready list */
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

/** Add a thread to manager list */
void psthread_add_manager(pstid_t psthrid)
{
	psthread_data_t *pt;

	pt = (psthread_data_t *) psthrid;

	futex_down(&psthread_futex);
	list_append(&pt->link, &manager_list);
	futex_up(&psthread_futex);
}

/** Remove one manager from manager list */
void psthread_remove_manager()
{
	futex_down(&psthread_futex);
	if (list_empty(&manager_list)) {
		futex_up(&psthread_futex);
		return;
	}
	list_remove(manager_list.next);
	futex_up(&psthread_futex);
}

/** Return thread id of current running thread */
pstid_t psthread_get_id(void)
{
	return (pstid_t)__tcb_get()->pst_data;
}

/** Disable preemption 
 *
 * If the thread wants to send several message in row and does not want
 * to be preempted, it should start async_serialize_start() in the beginning
 * of communication and async_serialize_end() in the end. If it is a
 * true multithreaded application, it should protect the communication channel
 * by a futex as well. Interrupt messages will can still be preempted.
 */
void psthread_inc_sercount(void)
{
	serialization_count++;
}

void psthread_dec_sercount(void)
{
	serialization_count--;
}


/** @}
 */
