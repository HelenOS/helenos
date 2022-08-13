/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ns
 * @{
 */

#ifndef NS_TASK_H__
#define NS_TASK_H__

#include <ipc/common.h>
#include <abi/proc/task.h>

extern errno_t task_init(void);
extern void process_pending_wait(void);

extern void wait_for_task(task_id_t, ipc_call_t *);

extern errno_t ns_task_id_intro(ipc_call_t *);
extern errno_t ns_task_disconnect(ipc_call_t *);
extern errno_t ns_task_retval(ipc_call_t *);

#endif

/**
 * @}
 */
