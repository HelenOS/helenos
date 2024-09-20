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

#include <libc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libarch/faddr.h>
#include <fibril.h>
#include <stack.h>
#include <str.h>
#include <async.h>
#include <errno.h>
#include <as.h>

#include "../private/thread.h"
#include "../private/fibril.h"

/** Main thread function.
 *
 * This function is called from __thread_entry() and is used
 * to call the thread's implementing function and perform cleanup
 * and exit when thread returns back.
 *
 * @param arg Fibril pointer.
 *
 */
static void __thread_main(void *arg)
{
	fibril_t *fibril = arg;

	assert(!__tcb_is_set());
	assert(fibril);

	__tcb_set(fibril->tcb);

	fibril->func(fibril->arg);
	/*
	 * XXX: we cannot free the userspace stack while running on it
	 *
	 * free(uarg->uspace_stack);
	 * free(uarg);
	 */

	fibril_teardown(fibril);
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
errno_t thread_create(errno_t (*func)(void *), void *arg, const char *name)
{
	fibril_t *fibril = fibril_alloc();
	if (!fibril)
		return ENOMEM;

	fibril->func = func;
	fibril->arg = arg;

	size_t stack_size = stack_size_get();
	void *stack = as_area_create(AS_AREA_ANY, stack_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);
	if (stack == AS_MAP_FAILED) {
		fibril_teardown(fibril);
		return ENOMEM;
	}

	uintptr_t sp = arch_thread_prepare(stack, stack_size, __thread_main,
	    fibril);

	errno_t rc = (errno_t) __SYSCALL4(SYS_THREAD_CREATE,
	    (sysarg_t) FADDR(__thread_entry), sp,
	    (sysarg_t) name, (sysarg_t) str_size(name));

	if (rc != EOK) {
		/*
		 * Failed to create a new thread.
		 * Free up the allocated data.
		 */
		as_area_destroy(stack);
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
void thread_usleep(usec_t usec)
{
	(void) __SYSCALL1(SYS_THREAD_USLEEP, usec);
}

/** Wait unconditionally for specified number of seconds
 *
 */
void thread_sleep(sec_t sec)
{
	/*
	 * Sleep in 1000 second steps to support full argument range
	 */
	while (sec > 0) {
		unsigned int period = (sec > 1000) ? 1000 : sec;

		thread_usleep(SEC2USEC(period));
		sec -= period;
	}
}

/** @}
 */
