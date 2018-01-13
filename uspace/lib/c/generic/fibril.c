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

#include <adt/list.h>
#include <fibril.h>
#include <thread.h>
#include <stack.h>
#include <tls.h>
#include <stdlib.h>
#include <abi/mm/as.h>
#include <as.h>
#include <stdio.h>
#include <libarch/barrier.h>
#include <libarch/faddr.h>
#include <futex.h>
#include <assert.h>
#include <async.h>

#ifdef FUTEX_UPGRADABLE
#include <rcu.h>
#endif

/**
 * This futex serializes access to ready_list,
 * manager_list and fibril_list.
 */
static futex_t fibril_futex = FUTEX_INITIALIZER;

static LIST_INITIALIZE(ready_list);
static LIST_INITIALIZE(manager_list);
static LIST_INITIALIZE(fibril_list);

/** Function that spans the whole life-cycle of a fibril.
 *
 * Each fibril begins execution in this function. Then the function implementing
 * the fibril logic is called.  After its return, the return value is saved.
 * The fibril then switches to another fibril, which cleans up after it.
 *
 */
static void fibril_main(void)
{
	fibril_t *fibril = __tcb_get()->fibril_data;

#ifdef FUTEX_UPGRADABLE
	rcu_register_fibril();
#endif
	
	/* Call the implementing function. */
	fibril->retval = fibril->func(fibril->arg);
	
	futex_down(&async_futex);
	fibril_switch(FIBRIL_FROM_DEAD);
	/* Not reached */
}

/** Setup fibril information into TCB structure
 *
 */
fibril_t *fibril_setup(void)
{
	tcb_t *tcb = tls_make();
	if (!tcb)
		return NULL;
	
	fibril_t *fibril = malloc(sizeof(fibril_t));
	if (!fibril) {
		tls_free(tcb);
		return NULL;
	}
	
	tcb->fibril_data = fibril;
	fibril->tcb = tcb;
	
	fibril->func = NULL;
	fibril->arg = NULL;
	fibril->stack = NULL;
	fibril->clean_after_me = NULL;
	fibril->retval = 0;
	fibril->flags = 0;
	
	fibril->waits_for = NULL;

	fibril->switches = 0;

	/*
	 * We are called before __tcb_set(), so we need to use
	 * futex_down/up() instead of futex_lock/unlock() that
	 * may attempt to access TLS.
	 */
	futex_down(&fibril_futex);
	list_append(&fibril->all_link, &fibril_list);
	futex_up(&fibril_futex);
	
	return fibril;
}

void fibril_teardown(fibril_t *fibril, bool locked)
{	
	if (!locked)
		futex_lock(&fibril_futex);
	list_remove(&fibril->all_link);
	if (!locked)
		futex_unlock(&fibril_futex);
	tls_free(fibril->tcb);
	free(fibril);
}

/** Switch from the current fibril.
 *
 * If stype is FIBRIL_TO_MANAGER or FIBRIL_FROM_DEAD, the async_futex must
 * be held.
 *
 * @param stype Switch type. One of FIBRIL_PREEMPT, FIBRIL_TO_MANAGER,
 *              FIBRIL_FROM_MANAGER, FIBRIL_FROM_DEAD. The parameter
 *              describes the circumstances of the switch.
 *
 * @return 0 if there is no ready fibril,
 * @return 1 otherwise.
 *
 */
int fibril_switch(fibril_switch_type_t stype)
{
	futex_lock(&fibril_futex);

	switch (stype) {
	case FIBRIL_PREEMPT:
	case FIBRIL_FROM_MANAGER:
		if (list_empty(&ready_list)) {
			futex_unlock(&fibril_futex);
			return 0;
		}
		break;
	case FIBRIL_TO_MANAGER:
	case FIBRIL_FROM_DEAD:
		/* Make sure the async_futex is held. */
		assert((atomic_signed_t) async_futex.val.count <= 0);

		/* If we are going to manager and none exists, create it */
		while (list_empty(&manager_list)) {
			futex_unlock(&fibril_futex);
			async_create_manager();
			futex_lock(&fibril_futex);
		}
		break;
	}
	
	fibril_t *srcf = __tcb_get()->fibril_data;
	if (stype != FIBRIL_FROM_DEAD) {
		
		/* Save current state */
		if (!context_save(&srcf->ctx)) {
			if (srcf->clean_after_me) {
				/*
				 * Cleanup after the dead fibril from which we
				 * restored context here.
				 */
				void *stack = srcf->clean_after_me->stack;
				if (stack) {
					/*
					 * This check is necessary because a
					 * thread could have exited like a
					 * normal fibril using the
					 * FIBRIL_FROM_DEAD switch type. In that
					 * case, its fibril will not have the
					 * stack member filled.
					 */
					as_area_destroy(stack);
				}
				fibril_teardown(srcf->clean_after_me, true);
				srcf->clean_after_me = NULL;
			}
			
			return 1;	/* futex_unlock already done here */
		}
		
		/* Put the current fibril into the correct run list */
		switch (stype) {
		case FIBRIL_PREEMPT:
			list_append(&srcf->link, &ready_list);
			break;
		case FIBRIL_FROM_MANAGER:
			list_append(&srcf->link, &manager_list);
			break;
		default:
			assert(stype == FIBRIL_TO_MANAGER);

			srcf->switches++;

			/*
			 * Don't put the current fibril into any list, it should
			 * already be somewhere, or it will be lost.
			 */
			break;
		}
	}
	
	fibril_t *dstf;

	/* Choose a new fibril to run */
	switch (stype) {
	case FIBRIL_TO_MANAGER:
	case FIBRIL_FROM_DEAD:
		dstf = list_get_instance(list_first(&manager_list), fibril_t,
		    link);
		
		if (stype == FIBRIL_FROM_DEAD) 
			dstf->clean_after_me = srcf;
		break;
	default:
		dstf = list_get_instance(list_first(&ready_list), fibril_t,
		    link);
		break;
	}

	list_remove(&dstf->link);
	
	futex_unlock(&fibril_futex);
	
#ifdef FUTEX_UPGRADABLE
	if (stype == FIBRIL_FROM_DEAD) {
		rcu_deregister_fibril();
	}
#endif
	
	context_restore(&dstf->ctx);
	/* not reached */
}

/** Create a new fibril.
 *
 * @param func Implementing function of the new fibril.
 * @param arg Argument to pass to func.
 * @param stksz Stack size in bytes.
 *
 * @return 0 on failure or TLS of the new fibril.
 *
 */
fid_t fibril_create_generic(errno_t (*func)(void *), void *arg, size_t stksz)
{
	fibril_t *fibril;
	
	fibril = fibril_setup();
	if (fibril == NULL)
		return 0;
	
	size_t stack_size = (stksz == FIBRIL_DFLT_STK_SIZE) ?
	    stack_size_get() : stksz;
	fibril->stack = as_area_create(AS_AREA_ANY, stack_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);
	if (fibril->stack == (void *) -1) {
		fibril_teardown(fibril, false);
		return 0;
	}
	
	fibril->func = func;
	fibril->arg = arg;

	context_save(&fibril->ctx);
	context_set(&fibril->ctx, FADDR(fibril_main), fibril->stack,
	    stack_size, fibril->tcb);

	return (fid_t) fibril;
}

/** Delete a fibril that has never run.
 *
 * Free resources of a fibril that has been created with fibril_create()
 * but never readied using fibril_add_ready().
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 */
void fibril_destroy(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;
	
	as_area_destroy(fibril->stack);
	fibril_teardown(fibril, false);
}

/** Add a fibril to the ready list.
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 *
 */
void fibril_add_ready(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;
	
	futex_lock(&fibril_futex);
	list_append(&fibril->link, &ready_list);
	futex_unlock(&fibril_futex);
}

/** Add a fibril to the manager list.
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 *
 */
void fibril_add_manager(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;
	
	futex_lock(&fibril_futex);
	list_append(&fibril->link, &manager_list);
	futex_unlock(&fibril_futex);
}

/** Remove one manager from the manager list. */
void fibril_remove_manager(void)
{
	futex_lock(&fibril_futex);
	if (!list_empty(&manager_list))
		list_remove(list_first(&manager_list));
	futex_unlock(&fibril_futex);
}

/** Return fibril id of the currently running fibril.
 *
 * @return fibril ID of the currently running fibril.
 *
 */
fid_t fibril_get_id(void)
{
	return (fid_t) __tcb_get()->fibril_data;
}

/** @}
 */
