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

#define atomic_inc(x)	((void) atomic_add(x, 1))
#define atomic_dec(x)	((void) atomic_add(x, -1))

#define atomic_inc_pre(x) (atomic_add(x, 1) - 1)
#define atomic_dec_pre(x) (atomic_add(x, -1) + 1)

#define atomic_inc_post(x) atomic_add(x, 1)
#define atomic_dec_post(x) atomic_add(x, -1)


typedef struct { volatile __u32 count; } atomic_t;

/* Atomic addition of immediate value.
 *
 * @param val Memory location to which will be the immediate value added.
 * @param i Signed immediate that will be added to *val.
 *
 * @return Value after addition.
 */
static inline count_t atomic_add(atomic_t *val, int i)
{
	count_t tmp, v;

	__asm__ volatile (
		"1:\n"
		"	ll %0, %1\n"
		"	addiu %0, %0, %3\n"	/* same as addi, but never traps on overflow */
		"       move %2, %0\n"
		"	sc %0, %1\n"
		"	beq %0, %4, 1b\n"	/* if the atomic operation failed, try again */
		/*	nop	*/		/* nop is inserted automatically by compiler */
		: "=r" (tmp), "=m" (val->count), "=r" (v)
		: "i" (i), "i" (0)
		);

	return v;
}

/* Reads/writes are atomic on mips for 4-bytes */

static inline void atomic_set(atomic_t *val, __u32 i)
{
	val->count = i;
}

static inline __u32 atomic_get(atomic_t *val)
{
	return val->count;
}

#endif
