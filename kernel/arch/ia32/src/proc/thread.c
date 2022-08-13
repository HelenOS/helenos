/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_proc
 * @{
 */
/** @file
 */

#include <proc/thread.h>

/** Perform ia32 specific thread initialization.
 *
 * @param t Thread to be initialized.
 */
errno_t thread_create_arch(thread_t *t, thread_flags_t flags)
{
	return EOK;
}

/** @}
 */
