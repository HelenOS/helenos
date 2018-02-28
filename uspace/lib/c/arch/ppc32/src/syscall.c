/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup libcppc32 ppc32
  * @brief ppc32 architecture dependent parts of libc
  * @ingroup lc
 * @{
 */
/** @file
 */

#include <libc.h>

sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6, const syscall_t id)
{
	register sysarg_t __ppc32_reg_r3 asm("3") = p1;
	register sysarg_t __ppc32_reg_r4 asm("4") = p2;
	register sysarg_t __ppc32_reg_r5 asm("5") = p3;
	register sysarg_t __ppc32_reg_r6 asm("6") = p4;
	register sysarg_t __ppc32_reg_r7 asm("7") = p5;
	register sysarg_t __ppc32_reg_r8 asm("8") = p6;
	register sysarg_t __ppc32_reg_r9 asm("9") = id;

	asm volatile (
		"sc\n"
		: "=r" (__ppc32_reg_r3)
		: "r" (__ppc32_reg_r3),
		  "r" (__ppc32_reg_r4),
		  "r" (__ppc32_reg_r5),
		  "r" (__ppc32_reg_r6),
		  "r" (__ppc32_reg_r7),
		  "r" (__ppc32_reg_r8),
		  "r" (__ppc32_reg_r9)
	);

	return __ppc32_reg_r3;
}

/** @}
 */
