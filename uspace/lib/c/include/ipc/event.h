/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_EVENT_H_
#define _LIBC_IPC_EVENT_H_

#include <abi/ipc/event.h>
#include <types/common.h>

extern errno_t ipc_event_subscribe(event_type_t, sysarg_t);
extern errno_t ipc_event_task_subscribe(event_task_type_t, sysarg_t);
extern errno_t ipc_event_unsubscribe(event_type_t);
extern errno_t ipc_event_task_unsubscribe(event_task_type_t);
extern errno_t ipc_event_unmask(event_type_t);
extern errno_t ipc_event_task_unmask(event_task_type_t);

#endif

/** @}
 */
