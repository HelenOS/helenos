/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_LIBC_H_
#define _LIBC_LIBC_H_

#include <stdint.h>
#include <abi/syscall.h>
#include <libarch/syscall.h>

#ifdef __32_BITS__

/** Explicit 64-bit arguments passed to syscalls. */
typedef uint64_t sysarg64_t;

#endif /* __32_BITS__ */

#define __SYSCALL0(id) \
	__syscall0(0, 0, 0, 0, 0, 0, id)
#define __SYSCALL1(id, p1) \
	__syscall1(p1, 0, 0, 0, 0, 0, id)
#define __SYSCALL2(id, p1, p2) \
	__syscall2(p1, p2, 0, 0, 0, 0, id)
#define __SYSCALL3(id, p1, p2, p3) \
	__syscall3(p1, p2, p3, 0, 0, 0, id)
#define __SYSCALL4(id, p1, p2, p3, p4) \
	__syscall4(p1, p2, p3, p4, 0, 0, id)
#define __SYSCALL5(id, p1, p2, p3, p4, p5) \
	__syscall5(p1, p2, p3, p4, p5, 0, id)
#define __SYSCALL6(id, p1, p2, p3, p4, p5, p6) \
	__syscall6(p1, p2, p3, p4, p5, p6, id)

extern void __libc_fini(void);

#endif

/** @}
 */
