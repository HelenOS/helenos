/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Userspace bootstrap thread.
 *
 * This file contains uinit kernel thread wich is used to start every
 * userspace thread including threads created by SYS_THREAD_CREATE syscall.
 *
 * @see SYS_THREAD_CREATE
 */

#include <main/uinit.h>
#include <typedefs.h>
#include <proc/thread.h>
#include <userspace.h>
#include <stdlib.h>
#include <arch.h>
#include <udebug/udebug.h>

/** Thread used to bring up userspace thread.
 *
 * @param arg Pointer to structure containing userspace entry and stack
 *     addresses.
 */
void uinit(void *arg)
{
	/*
	 * So far, we don't have a use for joining userspace threads so we
	 * immediately detach each uinit thread. If joining of userspace threads
	 * is required, some userspace API based on the kernel mechanism will
	 * have to be implemented. Moreover, garbage collecting of threads that
	 * didn't detach themselves and nobody else joined them will have to be
	 * deployed for the event of forceful task termination.
	 */
	thread_detach(THREAD);

#ifdef CONFIG_UDEBUG
	udebug_stoppable_end();
#endif

	uspace_arg_t *uarg = arg;
	uspace_arg_t local_uarg;

	local_uarg.uspace_entry = uarg->uspace_entry;
	local_uarg.uspace_stack = uarg->uspace_stack;
	local_uarg.uspace_stack_size = uarg->uspace_stack_size;
	local_uarg.uspace_uarg = uarg->uspace_uarg;
	local_uarg.uspace_thread_function = USPACE_NULL;
	local_uarg.uspace_thread_arg = USPACE_NULL;

	free(uarg);

	userspace(&local_uarg);
}

/** @}
 */
