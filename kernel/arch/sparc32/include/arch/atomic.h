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

/** @addtogroup sparc32
 * @{
 */
/** @file
 */

#ifndef KERN_sparc32_ATOMIC_H_
#define KERN_sparc32_ATOMIC_H_

#include <typedefs.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <preemption.h>
#include <verify.h>
#include <trace.h>

NO_TRACE ATOMIC static inline void atomic_inc(atomic_t *val)
    WRITES(&val->count)
    REQUIRES_EXTENT_MUTABLE(val)
    REQUIRES(val->count < ATOMIC_COUNT_MAX)
{
	// FIXME: Isn't there any intrinsic atomic operation?
	ipl_t ipl = interrupts_disable();
	val->count++;
	interrupts_restore(ipl);
}

NO_TRACE ATOMIC static inline void atomic_dec(atomic_t *val)
    WRITES(&val->count)
    REQUIRES_EXTENT_MUTABLE(val)
    REQUIRES(val->count > ATOMIC_COUNT_MIN)
{
	// FIXME: Isn't there any intrinsic atomic operation?
	ipl_t ipl = interrupts_disable();
	val->count--;
	interrupts_restore(ipl);
}

NO_TRACE ATOMIC static inline atomic_count_t atomic_postinc(atomic_t *val)
    WRITES(&val->count)
    REQUIRES_EXTENT_MUTABLE(val)
    REQUIRES(val->count < ATOMIC_COUNT_MAX)
{
	// FIXME: Isn't there any intrinsic atomic operation?
	
	ipl_t ipl = interrupts_disable();
	atomic_count_t prev = val->count;
	
	val->count++;
	interrupts_restore(ipl);
	return prev;
}

NO_TRACE ATOMIC static inline atomic_count_t atomic_postdec(atomic_t *val)
    WRITES(&val->count)
    REQUIRES_EXTENT_MUTABLE(val)
    REQUIRES(val->count > ATOMIC_COUNT_MIN)
{
	// FIXME: Isn't there any intrinsic atomic operation?
	
	ipl_t ipl = interrupts_disable();
	atomic_count_t prev = val->count;
	
	val->count--;
	interrupts_restore(ipl);
	return prev;
}

#define atomic_preinc(val)  (atomic_postinc(val) + 1)
#define atomic_predec(val)  (atomic_postdec(val) - 1)

NO_TRACE ATOMIC static inline atomic_count_t test_and_set(atomic_t *val)
    WRITES(&val->count)
    REQUIRES_EXTENT_MUTABLE(val)
{
	atomic_count_t prev;
	volatile uintptr_t ptr = (uintptr_t) &val->count;
	
	asm volatile (
		"ldstub [%[ptr]] %[prev]\n"
		: [prev] "=r" (prev)
		: [ptr] "r" (ptr)
		: "memory"
	);
	
	return prev;
}

NO_TRACE static inline void atomic_lock_arch(atomic_t *val)
    WRITES(&val->count)
    REQUIRES_EXTENT_MUTABLE(val)
{
	atomic_count_t tmp1 = 0;
	
	volatile uintptr_t ptr = (uintptr_t) &val->count;
	
	preemption_disable();
	
	asm volatile (
		"0:\n"
			"ldstub %0, %1\n"
			"tst %1\n"
			"be 2f\n"
			"nop\n"
		"1:\n"
			"ldub %0, %1\n"
			"tst %1\n"
			"bne 1b\n"
			"nop\n"
			"ba,a 0b\n"
		"2:\n"
		: "+m" (*((atomic_count_t *) ptr)),
		  "+r" (tmp1)
		: "r" (0)
	);
	
	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();
}

#endif

/** @}
 */
