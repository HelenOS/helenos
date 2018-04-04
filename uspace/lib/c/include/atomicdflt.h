/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_ATOMICDFLT_H_
#define LIBC_ATOMICDFLT_H_

#ifndef LIBC_ARCH_ATOMIC_H_
#error This file cannot be included directly, include atomic.h instead.
#endif

#include <types/common.h>
#include <stdbool.h>

typedef struct atomic {
	volatile atomic_count_t count;
} atomic_t;

static inline void atomic_set(atomic_t *val, atomic_count_t i)
{
	val->count = i;
}

static inline atomic_count_t atomic_get(atomic_t *val)
{
	return val->count;
}

#ifndef CAS
static inline bool cas(atomic_t *val, atomic_count_t ov, atomic_count_t nv)
{
	return __sync_bool_compare_and_swap(&val->count, ov, nv);
}
#endif

#endif

/** @}
 */
