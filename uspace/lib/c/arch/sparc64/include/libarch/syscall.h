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

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef LIBC_sparc64_SYSCALL_H_
#define LIBC_sparc64_SYSCALL_H_

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
