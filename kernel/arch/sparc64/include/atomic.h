/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_ATOMIC_H_
#define KERN_sparc64_ATOMIC_H_

#include <arch/barrier.h>
#include <arch/types.h>
#include <preemption.h>

/** Atomic add operation.
 *
 * Use atomic compare and swap operation to atomically add signed value.
 *
 * @param val Atomic variable.
 * @param i Signed value to be added.
 *
 * @return Value of the atomic variable as it existed before addition.
 */
static inline long atomic_add(atomic_t *val, int i)
{
	uint64_t a, b;

	do {
		volatile uintptr_t x = (uint64_t) &val->count;

		a = *((uint64_t *) x);
		b = a + i;
		asm volatile ("casx %0, %2, %1\n" : "+m" (*((uint64_t *)x)),
		    "+r" (b) : "r" (a));
	} while (a != b);

	return a;
}

static inline long atomic_preinc(atomic_t *val)
{
	return atomic_add(val, 1) + 1;
}

static inline long atomic_postinc(atomic_t *val)
{
	return atomic_add(val, 1);
}

static inline long atomic_predec(atomic_t *val)
{
	return atomic_add(val, -1) - 1;
}

static inline long atomic_postdec(atomic_t *val)
{
	return atomic_add(val, -1);
}

static inline void atomic_inc(atomic_t *val)
{
	(void) atomic_add(val, 1);
}

static inline void atomic_dec(atomic_t *val)
{
	(void) atomic_add(val, -1);
}

static inline long test_and_set(atomic_t *val)
{
	uint64_t v = 1;
	volatile uintptr_t x = (uint64_t) &val->count;

	asm volatile ("casx %0, %2, %1\n" : "+m" (*((uint64_t *) x)),
	    "+r" (v) : "r" (0));

	return v;
}

static inline void atomic_lock_arch(atomic_t *val)
{
	uint64_t tmp1 = 1;
	uint64_t tmp2 = 0;

	volatile uintptr_t x = (uint64_t) &val->count;

	preemption_disable();

	asm volatile (
	"0:\n"
		"casx %0, %3, %1\n"
		"brz %1, 2f\n"
		"nop\n"
	"1:\n"
		"ldx %0, %2\n"
		"brz %2, 0b\n"
		"nop\n"
		"ba %%xcc, 1b\n"
		"nop\n"
	"2:\n"
		: "+m" (*((uint64_t *) x)), "+r" (tmp1), "+r" (tmp2) : "r" (0)
	);
	
	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();
}

#endif

/** @}
 */
