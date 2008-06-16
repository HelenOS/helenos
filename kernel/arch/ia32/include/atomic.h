/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32	
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_ATOMIC_H_
#define KERN_ia32_ATOMIC_H_

#include <arch/types.h>
#include <arch/barrier.h>
#include <preemption.h>

static inline void atomic_inc(atomic_t *val) {
#ifdef CONFIG_SMP
	asm volatile ("lock incl %0\n" : "+m" (val->count));
#else
	asm volatile ("incl %0\n" : "+m" (val->count));
#endif /* CONFIG_SMP */
}

static inline void atomic_dec(atomic_t *val) {
#ifdef CONFIG_SMP
	asm volatile ("lock decl %0\n" : "+m" (val->count));
#else
	asm volatile ("decl %0\n" : "+m" (val->count));
#endif /* CONFIG_SMP */
}

static inline long atomic_postinc(atomic_t *val) 
{
	long r = 1;

	asm volatile (
		"lock xaddl %1, %0\n"
		: "+m" (val->count), "+r" (r)
	);

	return r;
}

static inline long atomic_postdec(atomic_t *val) 
{
	long r = -1;
	
	asm volatile (
		"lock xaddl %1, %0\n"
		: "+m" (val->count), "+r"(r)
	);
	
	return r;
}

#define atomic_preinc(val) (atomic_postinc(val) + 1)
#define atomic_predec(val) (atomic_postdec(val) - 1)

static inline uint32_t test_and_set(atomic_t *val) {
	uint32_t v;
	
	asm volatile (
		"movl $1, %0\n"
		"xchgl %0, %1\n"
		: "=r" (v),"+m" (val->count)
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
#ifdef CONFIG_HT
		"pause\n" /* Pentium 4's HT love this instruction */
#endif
		"mov %0, %1\n"
		"testl %1, %1\n"
		"jnz 0b\n"       /* lightweight looping on locked spinlock */
		
		"incl %1\n"      /* now use the atomic operation */
		"xchgl %0, %1\n"	
		"testl %1, %1\n"
		"jnz 0b\n"
                : "+m" (val->count), "=r"(tmp)
	);
	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();
}

#endif

/** @}
 */
