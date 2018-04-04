/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_ATOMIC_H_
#define KERN_mips32_ATOMIC_H_

#include <trace.h>

#define atomic_inc(x)  ((void) atomic_add(x, 1))
#define atomic_dec(x)  ((void) atomic_add(x, -1))

#define atomic_postinc(x)  (atomic_add(x, 1) - 1)
#define atomic_postdec(x)  (atomic_add(x, -1) + 1)

#define atomic_preinc(x)  atomic_add(x, 1)
#define atomic_predec(x)  atomic_add(x, -1)

/* Atomic addition of immediate value.
 *
 * @param val Memory location to which will be the immediate value added.
 * @param i Signed immediate that will be added to *val.
 *
 * @return Value after addition.
 *
 */
NO_TRACE static inline atomic_count_t atomic_add(atomic_t *val,
    atomic_count_t i)
{
	atomic_count_t tmp;
	atomic_count_t v;

	asm volatile (
	    "1:\n"
	    "	ll %0, %1\n"
	    "	addu %0, %0, %3\n"  /* same as addi, but never traps on overflow */
	    "	move %2, %0\n"
	    "	sc %0, %1\n"
	    "	beq %0, %4, 1b\n"   /* if the atomic operation failed, try again */
	    "	nop\n"
	    : "=&r" (tmp),
	      "+m" (val->count),
	      "=&r" (v)
	    : "r" (i),
	      "i" (0)
	);

	return v;
}

NO_TRACE static inline atomic_count_t test_and_set(atomic_t *val)
{
	atomic_count_t tmp;
	atomic_count_t v;

	asm volatile (
	    "1:\n"
	    "	ll %2, %1\n"
	    "	bnez %2, 2f\n"
	    "	li %0, %3\n"
	    "	sc %0, %1\n"
	    "	beqz %0, 1b\n"
	    "	nop\n"
	    "2:\n"
	    : "=&r" (tmp),
	      "+m" (val->count),
	      "=&r" (v)
	    : "i" (1)
	);

	return v;
}

NO_TRACE static inline void atomic_lock_arch(atomic_t *val)
{
	do {
		while (val->count)
			;
	} while (test_and_set(val));
}

#endif

/** @}
 */
