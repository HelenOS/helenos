/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_IPC_EVENT_H_
#define _ABI_IPC_EVENT_H_

/** Global events */
typedef enum event_type {
	/** New data available in kernel character buffer */
	EVENT_KIO = 0,
	/** Returning from kernel console to uspace */
	EVENT_KCONSOLE,
	/** A task/thread has faulted and will be terminated */
	EVENT_FAULT,
	/** New data available in kernel log */
	EVENT_KLOG,
	EVENT_END
} event_type_t;

/** Per-task events. */
typedef enum event_task_type {
	EVENT_TASK_STATE_CHANGE = EVENT_END,
	EVENT_TASK_END
} event_task_type_t;

#endif

/** @}
 */
