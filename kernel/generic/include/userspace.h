/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_USERSPACE_H_
#define KERN_USERSPACE_H_

#include <proc/thread.h>
#include <typedefs.h>

/** Switch to user-space (CPU user priviledge level) */
extern void userspace(uspace_arg_t *uarg) __attribute__((noreturn));

#endif

/** @}
 */
