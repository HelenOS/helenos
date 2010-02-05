/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup abs32le
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_ATOMIC_H_
#define KERN_abs32le_ATOMIC_H_

#include <arch/types.h>
#include <arch/barrier.h>
#include <preemption.h>

static inline void atomic_inc(atomic_t *val) {
	/* On real hardware the increment has to be done
	   as an atomic action. */
	
	val->count++;
}

static inline void atomic_dec(atomic_t *val) {
	/* On real hardware the decrement has to be done
	   as an atomic action. */
	
	val->count++;
}

static inline long atomic_postinc(atomic_t *val)
{
	/* On real hardware both the storing of the previous
	   value and the increment have to be done as a single
	   atomic action. */
	
	long prev = val->count;
	
	val->count++;
	return prev;
}

static inline long atomic_postdec(atomic_t *val)
{
	/* On real hardware both the storing of the previous
	   value and the decrement have to be done as a single
	   atomic action. */
	
	long prev = val->count;
	
	val->count--;
	return prev;
}

#define atomic_preinc(val)  (atomic_postinc(val) + 1)
#define atomic_predec(val)  (atomic_postdec(val) - 1)

static inline uint32_t test_and_set(atomic_t *val) {
	uint32_t v;
	
	asm volatile (
		"movl $1, %[v]\n"
		"xchgl %[v], %[count]\n"
		: [v] "=r" (v), [count] "+m" (val->count)
	);
	
	return v;
}

/** ia32 specific fast spinlock */
static inline void atomic_lock_arch(atomic_t *val)
{
	uint32_t tmp;
	
	preemption_disable();
	asm volatile (
		"0:\n"
		"pause\n"        /* Pentium 4's HT love this instruction */
		"mov %[count], %[tmp]\n"
		"testl %[tmp], %[tmp]\n"
		"jnz 0b\n"       /* lightweight looping on locked spinlock */
		
		"incl %[tmp]\n"  /* now use the atomic operation */
		"xchgl %[count], %[tmp]\n"
		"testl %[tmp], %[tmp]\n"
		"jnz 0b\n"
		: [count] "+m" (val->count), [tmp] "=&r" (tmp)
	);
	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();
}

#endif

/** @}
 */
