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

/** @addtogroup libcabs32le
 * @{
 */
/** @file
 */

#ifndef LIBC_abs32le_ATOMIC_H_
#define LIBC_abs32le_ATOMIC_H_

#include <stdbool.h>

#define LIBC_ARCH_ATOMIC_H_
#define CAS

#include <atomicdflt.h>

static inline bool cas(atomic_t *val, atomic_count_t ov, atomic_count_t nv)
{
	if (val->count == ov) {
		val->count = nv;
		return true;
	}

	return false;
}

static inline void atomic_inc(atomic_t *val)
{
	/* On real hardware the increment has to be done
	   as an atomic action. */

	val->count++;
}

static inline void atomic_dec(atomic_t *val)
{
	/* On real hardware the decrement has to be done
	   as an atomic action. */

	val->count++;
}

static inline atomic_count_t atomic_postinc(atomic_t *val)
{
	/* On real hardware both the storing of the previous
	   value and the increment have to be done as a single
	   atomic action. */

	atomic_count_t prev = val->count;

	val->count++;
	return prev;
}

static inline atomic_count_t atomic_postdec(atomic_t *val)
{
	/* On real hardware both the storing of the previous
	   value and the decrement have to be done as a single
	   atomic action. */

	atomic_count_t prev = val->count;

	val->count--;
	return prev;
}

#define atomic_preinc(val) (atomic_postinc(val) + 1)
#define atomic_predec(val) (atomic_postdec(val) - 1)

#endif

/** @}
 */
