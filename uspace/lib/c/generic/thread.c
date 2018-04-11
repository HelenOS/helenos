/*
 * Copyright (c) 2006 Jakub Jermar
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

#include <thread.h>
#include <libc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libarch/faddr.h>
#include <abi/proc/uarg.h>
#include <fibril.h>
#include <stack.h>
#include <str.h>
#include <async.h>
#include <errno.h>
#include <as.h>
#include "private/thread.h"

#ifdef FUTEX_UPGRADABLE
#include <rcu.h>
#endif


/** Main thread function.
 *
 * This function is called from __thread_entry() and is used
 * to call the thread's implementing function and perform cleanup
 * and exit when thread returns back.
 *
 * @param uarg Pointer to userspace argument structure.
 *
 */
void __thread_main(uspace_arg_t *uarg)
{
	fibril_t *fibril = fibril_setup();
	if (fibril == NULL)
		thread_exit(0);

	__tcb_set(fibril->tcb);

#ifdef FUTEX_UPGRADABLE
	rcu_register_fibril();
	futex_upgrade_all_and_wait();
#endif

	uarg->uspace_thread_function(uarg->uspace_thread_arg);
	/*
	 * XXX: we cannot free the userspace stack while running on it
	 *
	 * free(uarg->uspace_stack);
	 * free(uarg);
	 */

	/* If there is a manager, destroy it */
	async_destroy_manager();

#ifdef FUTEX_UPGRADABLE
	rcu_deregister_fibril();
#endif

	fibril_teardown(fibril, false);

	thread_exit(0);
}

/** Create userspace thread.
 *
 * This function creates new userspace thread and allocates userspace
 * stack and userspace argument structure for it.
 *
 * @param function Function implementing the thread.
 * @param arg Argument to be passed to thread.
 * @param name Symbolic name of the thread.
 * @param tid Thread ID of the newly created thread.
 *
 * @return Zero on success or a code from @ref errno.h on failure.
 */
errno_t thread_create(void (*function)(void *), void *arg, const char *name,
    thread_id_t *tid)
{
	uspace_arg_t *uarg =
	    (uspace_arg_t *) malloc(sizeof(uspace_arg_t));
	if (!uarg)
		return ENOMEM;

	size_t stack_size = stack_size_get();
	void *stack = as_area_create(AS_AREA_ANY, stack_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);
	if (stack == AS_MAP_FAILED) {
		free(uarg);
		return ENOMEM;
	}

	/* Make heap thread safe. */
	malloc_enable_multithreaded();

	uarg->uspace_entry = (void *) FADDR(__thread_entry);
	uarg->uspace_stack = stack;
	uarg->uspace_stack_size = stack_size;
	uarg->uspace_thread_function = function;
	uarg->uspace_thread_arg = arg;
	uarg->uspace_uarg = uarg;

	errno_t rc = (errno_t) __SYSCALL4(SYS_THREAD_CREATE, (sysarg_t) uarg,
	    (sysarg_t) name, (sysarg_t) str_size(name), (sysarg_t) tid);

	if (rc != EOK) {
		/*
		 * Failed to create a new thread.
		 * Free up the allocated data.
		 */
		as_area_destroy(stack);
		free(uarg);
	}

	return rc;
}

/** Terminate current thread.
 *
 * @param status Exit status. Currently not used.
 *
 */
void thread_exit(int status)
{
	__SYSCALL1(SYS_THREAD_EXIT, (sysarg_t) status);

	/* Unreachable */
	while (true)
		;
}

/** Detach thread.
 *
 * Currently not implemented.
 *
 * @param thread TID.
 */
void thread_detach(thread_id_t thread)
{
}

/** Join thread.
 *
 * Currently not implemented.
 *
 * @param thread TID.
 *
 * @return Thread exit status.
 */
errno_t thread_join(thread_id_t thread)
{
	return 0;
}

/** Get current thread ID.
 *
 * @return Current thread ID.
 */
thread_id_t thread_get_id(void)
{
	thread_id_t thread_id;

	(void) __SYSCALL1(SYS_THREAD_GET_ID, (sysarg_t) &thread_id);

	return thread_id;
}

/** Wait unconditionally for specified number of microseconds
 *
 */
int thread_usleep(useconds_t usec)
{
	(void) __SYSCALL1(SYS_THREAD_USLEEP, usec);
	return 0;
}

/** Wait unconditionally for specified number of seconds
 *
 */
unsigned int thread_sleep(unsigned int sec)
{
	/*
	 * Sleep in 1000 second steps to support
	 * full argument range
	 */

	while (sec > 0) {
		unsigned int period = (sec > 1000) ? 1000 : sec;

		thread_usleep(period * 1000000);
		sec -= period;
	}

	return 0;
}

/** @}
 */
