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

/** @addtogroup ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_ATOMIC_H_
#define KERN_ia64_ATOMIC_H_

#include <trace.h>

NO_TRACE static inline atomic_count_t test_and_set(atomic_t *val)
{
	atomic_count_t v;

	asm volatile (
	    "movl %[v] = 0x1;;\n"
	    "xchg8 %[v] = %[count], %[v];;\n"
	    : [v] "=r" (v),
	      [count] "+m" (val->count)
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

NO_TRACE static inline void atomic_inc(atomic_t *val)
{
	atomic_count_t v;

	asm volatile (
	    "fetchadd8.rel %[v] = %[count], 1\n"
	    : [v] "=r" (v),
	      [count] "+m" (val->count)
	);
}

NO_TRACE static inline void atomic_dec(atomic_t *val)
{
	atomic_count_t v;

	asm volatile (
	    "fetchadd8.rel %[v] = %[count], -1\n"
	    : [v] "=r" (v),
	      [count] "+m" (val->count)
	);
}

NO_TRACE static inline atomic_count_t atomic_preinc(atomic_t *val)
{
	atomic_count_t v;

	asm volatile (
	    "fetchadd8.rel %[v] = %[count], 1\n"
	    : [v] "=r" (v),
	      [count] "+m" (val->count)
	);

	return (v + 1);
}

NO_TRACE static inline atomic_count_t atomic_predec(atomic_t *val)
{
	atomic_count_t v;

	asm volatile (
	    "fetchadd8.rel %[v] = %[count], -1\n"
	    : [v] "=r" (v),
	      [count] "+m" (val->count)
	);

	return (v - 1);
}

NO_TRACE static inline atomic_count_t atomic_postinc(atomic_t *val)
{
	atomic_count_t v;

	asm volatile (
	    "fetchadd8.rel %[v] = %[count], 1\n"
	    : [v] "=r" (v),
	      [count] "+m" (val->count)
	);

	return v;
}

NO_TRACE static inline atomic_count_t atomic_postdec(atomic_t *val)
{
	atomic_count_t v;

	asm volatile (
	    "fetchadd8.rel %[v] = %[count], -1\n"
	    : [v] "=r" (v),
	      [count] "+m" (val->count)
	);

	return v;
}

#endif

/** @}
 */
