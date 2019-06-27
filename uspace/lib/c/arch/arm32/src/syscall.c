/*
 * Copyright (c) 2007 Pavel Jancik
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

/** @addtogroup libcarm32
 * @{
 */
/** @file
 *  @brief Syscall routine.
 */

#include <libc.h>

/** Syscall routine.
 *
 *  Stores p1-p4, id to r0-r4 registers and calls <code>swi</code>
 *  instruction. Returned value is read from r0 register.
 *
 *  @param p1 Parameter 1.
 *  @param p2 Parameter 2.
 *  @param p3 Parameter 3.
 *  @param p4 Parameter 4.
 *  @param id Number of syscall.
 *
 *  @return Syscall return value.
 */
sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6, const syscall_t id)
{
	register sysarg_t __arm_reg_r0 asm("r0") = p1;
	register sysarg_t __arm_reg_r1 asm("r1") = p2;
	register sysarg_t __arm_reg_r2 asm("r2") = p3;
	register sysarg_t __arm_reg_r3 asm("r3") = p4;
	register sysarg_t __arm_reg_r4 asm("r4") = p5;
	register sysarg_t __arm_reg_r5 asm("r5") = p6;
	register sysarg_t __arm_reg_r6 asm("r6") = id;

	asm volatile (
	    "swi 0"
	    : "=r" (__arm_reg_r0)
	    : "r" (__arm_reg_r0),
	      "r" (__arm_reg_r1),
	      "r" (__arm_reg_r2),
	      "r" (__arm_reg_r3),
	      "r" (__arm_reg_r4),
	      "r" (__arm_reg_r5),
	      "r" (__arm_reg_r6)
	    :
	      /*
	       * Clobber memory too as some arguments might be
	       * actually pointers.
	       */
	      "memory"
	);

	return __arm_reg_r0;
}

/** @}
 */
