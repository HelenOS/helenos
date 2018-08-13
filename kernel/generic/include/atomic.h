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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef KERN_ATOMIC_H_
#define KERN_ATOMIC_H_

#include <stdbool.h>
#include <typedefs.h>
#include <stdatomic.h>

typedef size_t atomic_count_t;
typedef ssize_t atomic_signed_t;

#define PRIua  "zu"          /**< Format for atomic_count_t. */

typedef struct {
	volatile atomic_size_t count;
} atomic_t;

static inline void atomic_set(atomic_t *val, atomic_count_t i)
{
	atomic_store(&val->count, i);
}

static inline atomic_count_t atomic_get(atomic_t *val)
{
	return atomic_load(&val->count);
}

static inline size_t atomic_predec(atomic_t *val)
{
	return atomic_fetch_sub(&val->count, 1) - 1;
}

static inline size_t atomic_preinc(atomic_t *val)
{
	return atomic_fetch_add(&val->count, 1) + 1;
}

static inline size_t atomic_postdec(atomic_t *val)
{
	return atomic_fetch_sub(&val->count, 1);
}

static inline size_t atomic_postinc(atomic_t *val)
{
	return atomic_fetch_add(&val->count, 1);
}

static inline void atomic_dec(atomic_t *val)
{
	atomic_fetch_sub(&val->count, 1);
}

static inline void atomic_inc(atomic_t *val)
{
	atomic_fetch_add(&val->count, 1);
}

#define local_atomic_exchange(var_addr, new_val) \
	atomic_exchange_explicit(var_addr, new_val, memory_order_relaxed)

static inline bool test_and_set(atomic_t *val)
{
	return atomic_exchange(&val->count, 1);
}


#endif

/** @}
 */
