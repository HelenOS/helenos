/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef _LIBC_riscv64_SYSCALL_H_
#define _LIBC_riscv64_SYSCALL_H_

#include <stdint.h>
#include <abi/syscall.h>
#include <types/common.h>

#define __syscall0  __syscall
#define __syscall1  __syscall
#define __syscall2  __syscall
#define __syscall3  __syscall
#define __syscall4  __syscall
#define __syscall5  __syscall
#define __syscall6  __syscall

extern sysarg_t __syscall(const sysarg_t, const sysarg_t, const sysarg_t,
    const sysarg_t, const sysarg_t, const sysarg_t, const syscall_t);

#endif

/** @}
 */
