/*
 * Copyright (C) 2005 Jakub Jermar
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

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef LIBC_sparc64_ATOMIC_H_
#define LIBC_sparc64_ATOMIC_H_

#include <types.h>

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
	volatile uint64_t x = (uint64_t) &val->count;

	__asm__ volatile (
		"0:\n"
		"ldx %0, %1\n"
		"add %1, %3, %2\n"
		"casx %0, %1, %2\n"
		"cmp %1, %2\n"
		"bne 0b\n"		/* The operation failed and must be attempted again if a != b. */
		"nop\n"
		: "=m" (*((uint64_t *)x)), "=r" (a), "=r" (b)
		: "r" (i)
	);

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

#endif

/** @}
 */
