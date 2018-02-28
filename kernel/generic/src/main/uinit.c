/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup main
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
#include <mm/slab.h>
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

	uspace_arg_t *uarg = (uspace_arg_t *) arg;
	uspace_arg_t local_uarg;

	local_uarg.uspace_entry = uarg->uspace_entry;
	local_uarg.uspace_stack = uarg->uspace_stack;
	local_uarg.uspace_stack_size = uarg->uspace_stack_size;
	local_uarg.uspace_uarg = uarg->uspace_uarg;
	local_uarg.uspace_thread_function = NULL;
	local_uarg.uspace_thread_arg = NULL;

	free(uarg);

	userspace(&local_uarg);
}

/** @}
 */
