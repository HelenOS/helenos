/*
 * Copyright (c) 2007 Michal Kebrt
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
 *  @brief Atomic operations.
 */

#ifndef LIBC_arm32_ATOMIC_H_
#define LIBC_arm32_ATOMIC_H_

#define LIBC_ARCH_ATOMIC_H_
#define CAS

#include <atomicdflt.h>
#include <stdbool.h>
#include <stdint.h>

extern uintptr_t *ras_page;

static inline bool cas(atomic_t *val, atomic_count_t ov, atomic_count_t nv)
{
	atomic_count_t ret = 0;

	/*
	 * The following instructions between labels 1 and 2 constitute a
	 * Restartable Atomic Seqeunce. Should the sequence be non-atomic,
	 * the kernel will restart it.
	 */
	asm volatile (
		"1:\n"
		"	adr %[ret], 1b\n"
		"	str %[ret], %[rp0]\n"
		"	adr %[ret], 2f\n"
		"	str %[ret], %[rp1]\n"
		"	ldr %[ret], %[addr]\n"
		"	cmp %[ret], %[ov]\n"
		"	streq %[nv], %[addr]\n"
		"2:\n"
		"	moveq %[ret], #1\n"
		"	movne %[ret], #0\n"
		: [ret] "+&r" (ret),
		  [rp0] "=m" (ras_page[0]),
		  [rp1] "=m" (ras_page[1]),
		  [addr] "+m" (val->count)
		: [ov] "r" (ov),
		  [nv] "r" (nv)
		: "memory"
	);

	ras_page[0] = 0;
	asm volatile (
		"" ::: "memory"
	);
	ras_page[1] = 0xffffffff;

	return ret != 0;
}

/** Atomic addition.
 *
 * @param val Where to add.
 * @param i   Value to be added.
 *
 * @return Value after addition.
 *
 */
static inline atomic_count_t atomic_add(atomic_t *val, atomic_count_t i)
{
	atomic_count_t ret = 0;

	/*
	 * The following instructions between labels 1 and 2 constitute a
	 * Restartable Atomic Seqeunce. Should the sequence be non-atomic,
	 * the kernel will restart it.
	 */
	asm volatile (
		"1:\n"
		"	adr %[ret], 1b\n"
		"	str %[ret], %[rp0]\n"
		"	adr %[ret], 2f\n"
		"	str %[ret], %[rp1]\n"
		"	ldr %[ret], %[addr]\n"
		"	add %[ret], %[ret], %[imm]\n"
		"	str %[ret], %[addr]\n"
		"2:\n"
		: [ret] "+&r" (ret),
		  [rp0] "=m" (ras_page[0]),
		  [rp1] "=m" (ras_page[1]),
		  [addr] "+m" (val->count)
		: [imm] "r" (i)
	);

	ras_page[0] = 0;
	asm volatile (
		"" ::: "memory"
	);
	ras_page[1] = 0xffffffff;

	return ret;
}


/** Atomic increment.
 *
 * @param val Variable to be incremented.
 *
 */
static inline void atomic_inc(atomic_t *val)
{
	atomic_add(val, 1);
}


/** Atomic decrement.
 *
 * @param val Variable to be decremented.
 *
 */
static inline void atomic_dec(atomic_t *val)
{
	atomic_add(val, -1);
}


/** Atomic pre-increment.
 *
 * @param val Variable to be incremented.
 * @return    Value after incrementation.
 *
 */
static inline atomic_count_t atomic_preinc(atomic_t *val)
{
	return atomic_add(val, 1);
}


/** Atomic pre-decrement.
 *
 * @param val Variable to be decremented.
 * @return    Value after decrementation.
 *
 */
static inline atomic_count_t atomic_predec(atomic_t *val)
{
	return atomic_add(val, -1);
}


/** Atomic post-increment.
 *
 * @param val Variable to be incremented.
 * @return    Value before incrementation.
 *
 */
static inline atomic_count_t atomic_postinc(atomic_t *val)
{
	return atomic_add(val, 1) - 1;
}


/** Atomic post-decrement.
 *
 * @param val Variable to be decremented.
 * @return    Value before decrementation.
 *
 */
static inline atomic_count_t atomic_postdec(atomic_t *val)
{
	return atomic_add(val, -1) + 1;
}


#endif

/** @}
 */
