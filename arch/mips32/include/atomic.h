/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#ifndef __mips32_ATOMIC_H__
#define __mips32_ATOMIC_H__

#include <arch/types.h>

#define atomic_inc(x)	(a_add(x,1))
#define atomic_dec(x)	(a_sub(x,1))

#define atomic_inc_pre(x) (a_add(x,1)-1)
#define atomic_dec_pre(x) (a_sub(x,1)+1)

#define atomic_inc_post(x) (a_add(x,1))
#define atomic_dec_post(x) (a_sub(x,1))


typedef volatile __u32 atomic_t;

/*
 * Atomic addition
 *
 * This case is harder, and we have to use the special LL and SC operations
 * to achieve atomicity. The instructions are similar to LW (load) and SW
 * (store), except that the LL (load-linked) instruction loads the address
 * of the variable to a special register and if another process writes to
 * the same location, the SC (store-conditional) instruction fails.
 
 Returns (*val)+i
 
 */
static inline atomic_t a_add(atomic_t *val, int i)
{
	atomic_t tmp, tmp2;

	asm volatile (
		"	.set	push\n"
		"	.set	noreorder\n"
		"	nop\n"
		"1:\n"
		"	ll	%0, %1\n"
		"	addu	%0, %0, %3\n"
		"       move    %2, %0\n"
		"	sc	%0, %1\n"
		"	beq	%0, 0x0, 1b\n"
		"	move    %0, %2\n"
		"	.set	pop\n"
		: "=&r" (tmp), "=o" (*val), "=r" (tmp2)
		: "r" (i)
		);
	return tmp;
}


/*
 * Atomic subtraction
 *
 * Implemented in the same manner as a_add, except we substract the value.

 Returns (*val)-i

 */
static inline atomic_t a_sub(atomic_t *val, int i)

{
	atomic_t tmp, tmp2;

	asm volatile (
		"	.set	push\n"
		"	.set	noreorder\n"
		"	nop\n"
		"1:\n"
		"	ll	%0, %1\n"
		"	subu	%0, %0, %3\n"
		"       move    %2, %0\n"
		"	sc	%0, %1\n"
		"	beq	%0, 0x0, 1b\n"
		"       move    %0, %2\n"
		"	.set	pop\n"
		: "=&r" (tmp), "=o" (*val), "=r" (tmp2)
		: "r" (i)
		);
	return tmp;
}


#endif
