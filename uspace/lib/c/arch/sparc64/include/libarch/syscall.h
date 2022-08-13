/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef _LIBC_sparc64_SYSCALL_H_
#define _LIBC_sparc64_SYSCALL_H_

#include <stdint.h>
#include <abi/syscall.h>
#include <types/common.h>

#define __syscall0	__syscall
#define __syscall1	__syscall
#define __syscall2	__syscall
#define __syscall3	__syscall
#define __syscall4	__syscall
#define __syscall5	__syscall
#define __syscall6	__syscall

static inline sysarg_t
__syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6, const syscall_t id)
{
	register uint64_t a1 asm("o0") = p1;
	register uint64_t a2 asm("o1") = p2;
	register uint64_t a3 asm("o2") = p3;
	register uint64_t a4 asm("o3") = p4;
	register uint64_t a5 asm("o4") = p5;
	register uint64_t a6 asm("o5") = p6;

	asm volatile (
	    "ta %7\n"
	    : "=r" (a1)
	    : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6),
	      "i" (id)
	    : "memory"
	);

	return a1;
}

#endif

/** @}
 */
