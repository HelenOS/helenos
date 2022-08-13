/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_SYSCALL_H_
#define KERN_SYSCALL_H_

#include <typedefs.h>
#include <abi/syscall.h>

typedef sysarg_t (*syshandler_t)(sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);

extern sysarg_t syscall_handler(sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

#endif

/** @}
 */
