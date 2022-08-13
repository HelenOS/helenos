/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_PROC_THREAD_H_
#define _ABI_PROC_THREAD_H_

#include <stdint.h>

typedef uint64_t thread_id_t;

/** Thread states */
typedef enum {
	/** It is an error, if thread is found in this state. */
	Invalid,
	/** State of a thread that is currently executing on some CPU. */
	Running,
	/** Thread in this state is waiting for an event. */
	Sleeping,
	/** State of threads in a run queue. */
	Ready,
	/** Threads are in this state before they are first readied. */
	Entering,
	/** After a thread calls thread_exit(), it is put into Exiting state. */
	Exiting,
	/** Threads that were not detached but exited are Lingering. */
	Lingering
} state_t;

#endif

/** @}
 */
