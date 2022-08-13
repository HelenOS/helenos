/*
 * SPDX-FileCopyrightText: 2018 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sync
 * @{
 */
/** @file
 */

#ifndef KERN_SYS_WAITQ_H_
#define KERN_SYS_WAITQ_H_

#include <typedefs.h>
#include <abi/cap.h>
#include <cap/cap.h>

extern kobject_ops_t waitq_kobject_ops;

extern void sys_waitq_init(void);

extern void sys_waitq_task_cleanup(void);

extern sys_errno_t sys_waitq_create(uspace_ptr_cap_waitq_handle_t);
extern sys_errno_t sys_waitq_sleep(cap_waitq_handle_t, uint32_t, unsigned int);
extern sys_errno_t sys_waitq_wakeup(cap_waitq_handle_t);
extern sys_errno_t sys_waitq_destroy(cap_waitq_handle_t);

#endif

/** @}
 */
