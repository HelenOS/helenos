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
 * @brief Syscall function declaration for architectures that don't
 *        inline syscalls or architectures that handle syscalls
 *        according to the number of arguments.
 */

#ifndef _LIBC_SYSCALL_H_
#define _LIBC_SYSCALL_H_

#ifndef LIBARCH_SYSCALL_GENERIC
#error You cannot include this file directly
#endif

#include <abi/syscall.h>
#include <types/common.h>

#define __syscall0  __syscall
#define __syscall1  __syscall
#define __syscall2  __syscall
#define __syscall3  __syscall
#define __syscall4  __syscall
#define __syscall5  __syscall
#define __syscall6  __syscall

extern sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2,
    const sysarg_t p3, const sysarg_t p4, const sysarg_t p5, const sysarg_t p6,
    const syscall_t id);

#endif

/** @}
 */
