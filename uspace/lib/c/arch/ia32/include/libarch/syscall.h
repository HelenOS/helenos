/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef _LIBC_ia32_SYSCALL_H_
#define _LIBC_ia32_SYSCALL_H_

#include <abi/syscall.h>
#include <types/common.h>

#define __syscall0  __syscall_fast_func
#define __syscall1  __syscall_fast_func
#define __syscall2  __syscall_fast_func
#define __syscall3  __syscall_fast_func
#define __syscall4  __syscall_fast_func
#define __syscall5  __syscall_slow
#define __syscall6  __syscall_slow

extern sysarg_t (*__syscall_fast_func)(const sysarg_t, const sysarg_t,
    const sysarg_t, const sysarg_t, const sysarg_t, const sysarg_t,
    const syscall_t);

extern sysarg_t __syscall_slow(const sysarg_t, const sysarg_t, const sysarg_t,
    const sysarg_t, const sysarg_t, const sysarg_t, const syscall_t);

#endif

/** @}
 */
