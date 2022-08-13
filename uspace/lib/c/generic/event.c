/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 * @}
 */

/** @addtogroup libc
 */
/** @file
 */

#include <libc.h>
#include <ipc/event.h>

/** Subscribe to event notifications.
 *
 * @param evno    Event type to subscribe.
 * @param imethod Use this interface and method for notifying me.
 *
 * @return Error code returned by the kernel.
 *
 */
errno_t ipc_event_subscribe(event_type_t evno, sysarg_t imethod)
{
	return (errno_t) __SYSCALL2(SYS_IPC_EVENT_SUBSCRIBE, (sysarg_t) evno,
	    (sysarg_t) imethod);
}

/** Subscribe to task event notifications.
 *
 * @param evno    Event type to subscribe.
 * @param imethod Use this interface and method for notifying me.
 *
 * @return Error code returned by the kernel.
 *
 */
errno_t ipc_event_task_subscribe(event_task_type_t evno, sysarg_t imethod)
{
	return (errno_t) __SYSCALL2(SYS_IPC_EVENT_SUBSCRIBE, (sysarg_t) evno,
	    (sysarg_t) imethod);
}

/** Unsubscribe from event notifications.
 *
 * @param evno    Event type to unsubscribe.
 *
 * @return Error code returned by the kernel.
 *
 */
errno_t ipc_event_unsubscribe(event_type_t evno)
{
	return (errno_t) __SYSCALL1(SYS_IPC_EVENT_UNSUBSCRIBE, (sysarg_t) evno);
}

/** Unsubscribe from task event notifications.
 *
 * @param evno    Event type to unsubscribe.
 *
 * @return Error code returned by the kernel.
 *
 */
errno_t ipc_event_task_unsubscribe(event_task_type_t evno)
{
	return (errno_t) __SYSCALL1(SYS_IPC_EVENT_UNSUBSCRIBE, (sysarg_t) evno);
}

/** Unmask event notifications.
 *
 * @param evno Event type to unmask.
 *
 * @return Error code returned by the kernel.
 *
 */
errno_t ipc_event_unmask(event_type_t evno)
{
	return (errno_t) __SYSCALL1(SYS_IPC_EVENT_UNMASK, (sysarg_t) evno);
}

/** Unmask task event notifications.
 *
 * @param evno Event type to unmask.
 *
 * @return Error code returned by the kernel.
 *
 */
errno_t ipc_event_task_unmask(event_task_type_t evno)
{
	return (errno_t) __SYSCALL1(SYS_IPC_EVENT_UNMASK, (sysarg_t) evno);
}

/** @}
 */
