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

#include <bool.h>

typedef struct atomic {
	volatile long count;
} atomic_t;

static inline void atomic_set(atomic_t *val, long i)
{
        val->count = i;
}

static inline long atomic_get(atomic_t *val)
{
        return val->count;
}

static inline bool cas(atomic_t *val, long ov, long nv)
{
	/* FIXME: is not atomic */
	if (val->count == ov) {
		val->count = nv;
		return true;
	}
	return false;
}

/** Atomic addition.
 *
 * @param val Where to add.
 * @param i   Value to be added.
 *
 * @return Value after addition.
 */
static inline long atomic_add(atomic_t *val, int i)
{
	int ret;
	volatile long * mem = &(val->count);

	/* FIXME: is not atomic, is broken */
	asm volatile (
	"1:\n"
		"ldr r2, [%1]\n"
		"add r3, r2, %2\n"
		"str r3, %0\n"
		"swp r3, r3, [%1]\n"
		"cmp r3, r2\n"
		"bne 1b\n"

		: "=m" (ret)
		: "r" (mem), "r" (i)
		: "r3", "r2"
	);

	return ret;
}


/** Atomic increment.
 *
 * @param val Variable to be incremented.
 */
static inline void atomic_inc(atomic_t *val)
{
	atomic_add(val, 1);
}


/** Atomic decrement.
 *
 * @param val Variable to be decremented.
 */
static inline void atomic_dec(atomic_t *val)
{
	atomic_add(val, -1);
}


/** Atomic pre-increment.
 *
 * @param val Variable to be incremented.
 * @return    Value after incrementation.
 */
static inline long atomic_preinc(atomic_t *val)
{
	return atomic_add(val, 1);
}


/** Atomic pre-decrement.
 *
 * @param val Variable to be decremented.
 * @return    Value after decrementation.
 */
static inline long atomic_predec(atomic_t *val)
{
	return atomic_add(val, -1);
}


/** Atomic post-increment.
 *
 * @param val Variable to be incremented.
 * @return    Value before incrementation.
 */
static inline long atomic_postinc(atomic_t *val)
{
	return atomic_add(val, 1) - 1;
}


/** Atomic post-decrement.
 *
 * @param val Variable to be decremented.
 * @return    Value before decrementation.
 */
static inline long atomic_postdec(atomic_t *val)
{
	return atomic_add(val, -1) + 1;
}


#endif

/** @}
 */
