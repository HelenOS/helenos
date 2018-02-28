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

/** @addtogroup libcmips32
 * @{
 */
/** @file
  * @ingroup libcmips32
 */

#include <libc.h>

sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6, const syscall_t id)
{
	register sysarg_t __mips_reg_a0 asm("$4") = p1;
	register sysarg_t __mips_reg_a1 asm("$5") = p2;
	register sysarg_t __mips_reg_a2 asm("$6") = p3;
	register sysarg_t __mips_reg_a3 asm("$7") = p4;
	register sysarg_t __mips_reg_t0 asm("$8") = p5;
	register sysarg_t __mips_reg_t1 asm("$9") = p6;
	register sysarg_t __mips_reg_v0 asm("$2") = id;

	asm volatile (
		"syscall\n"
		: "=r" (__mips_reg_v0)
		: "r" (__mips_reg_a0),
		  "r" (__mips_reg_a1),
		  "r" (__mips_reg_a2),
		  "r" (__mips_reg_a3),
		  "r" (__mips_reg_t0),
		  "r" (__mips_reg_t1),
		  "r" (__mips_reg_v0)
		/*
		 * We are a function call, although C
		 * does not know it.
		 */
		: "%ra"
	);

	return __mips_reg_v0;
}

/** @}
 */
