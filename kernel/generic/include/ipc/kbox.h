/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#ifndef KERN_IPC_KBOX_H_
#define KERN_IPC_KBOX_H_

#include <typedefs.h>

/** Kernel answerbox structure. */
typedef struct kbox {
	/** The answerbox itself. */
	answerbox_t box;
	/** Thread used to service the answerbox. */
	struct thread *thread;
	/** Kbox thread creation vs. begin of cleanup mutual exclusion. */
	mutex_t cleanup_lock;
	/** True if cleanup of kbox has already started. */
	bool finished;
} kbox_t;

extern errno_t ipc_connect_kbox(task_id_t, cap_phone_handle_t *);
extern void ipc_kbox_cleanup(void);

#endif

/** @}
 */
