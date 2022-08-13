/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#ifndef KERN_IPCRSC_H_
#define KERN_IPCRSC_H_

#include <proc/task.h>
#include <ipc/ipc.h>
#include <cap/cap.h>

extern kobject_ops_t phone_kobject_ops;

extern errno_t phone_alloc(task_t *, bool, cap_phone_handle_t *, kobject_t **);
extern void phone_dealloc(cap_phone_handle_t);

#endif

/** @}
 */
