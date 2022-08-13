/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_proc
 * @{
 */
/** @file
 */

#include <proc/thread.h>
#include <arch/interrupt.h>
#include <arch/kseg_struct.h>

/** Perform amd64 specific thread initialization.
 *
 * @param thread Thread to be initialized.
 *
 */
errno_t thread_create_arch(thread_t *thread, thread_flags_t flags)
{
	/*
	 * Kernel RSP can be precalculated at thread creation time.
	 */
	thread->arch.kstack_rsp =
	    (uintptr_t) &thread->kstack[PAGE_SIZE - sizeof(istate_t)];
	return EOK;
}

/** @}
 */
