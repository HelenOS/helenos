/*
 * Copyright (c) 2005 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
