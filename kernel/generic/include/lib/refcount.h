/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

/*
 * Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/*
 * Using atomics for reference counting efficiently is a little tricky,
 * so we define a unified API for this.
 */

#ifndef _LIBC_REFCOUNT_H_
#define _LIBC_REFCOUNT_H_

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>

/* Wrapped in a structure to prevent direct manipulation. */
typedef struct atomic_refcount {
	volatile atomic_int __cnt;
} atomic_refcount_t;

#define REFCOUNT_INITIALIZER() { \
	.__cnt = ATOMIC_VAR_INIT(0), \
}

static inline void refcount_init(atomic_refcount_t *rc)
{
	atomic_init(&rc->__cnt, 0);
}

/**
 * Increment a reference count.
 *
 * Calling this without already owning a reference is undefined behavior.
 * E.g. acquiring a reference through a shared mutable pointer requires that
 * the caller first locks the pointer itself (thereby acquiring the reference
 * inherent to the shared variable), and only then may call refcount_up().
 */
static inline void refcount_up(atomic_refcount_t *rc)
{
	// NOTE: We can use relaxed operation because acquiring a reference
	//       implies no ordering relationships. A reference-counted object
	//       still needs to be synchronized independently of the refcount.

	int old = atomic_fetch_add_explicit(&rc->__cnt, 1,
	    memory_order_relaxed);

	/* old < 0 indicates that the function is used incorrectly. */
	assert(old >= 0);
	(void) old;
}

/**
 * Try to upgrade a weak reference.
 * Naturally, this is contingent on another form of synchronization being used
 * to ensure that the object continues to exist while the weak reference is in
 * use.
 */
static inline bool refcount_try_up(atomic_refcount_t *rc)
{
	int cnt = atomic_load_explicit(&rc->__cnt, memory_order_relaxed);

	while (cnt >= 0) {
		if (atomic_compare_exchange_weak_explicit(&rc->__cnt, &cnt, cnt + 1,
		    memory_order_relaxed, memory_order_relaxed)) {
			return true;
		}
	}

	return false;
}

static inline bool refcount_unique(atomic_refcount_t *rc)
{
	int val = atomic_load_explicit(&rc->__cnt, memory_order_acquire);
	if (val < 0) {
		assert(val == -1);
	}

	return val <= 0;
}

/**
 * Decrement a reference count. Caller must own the reference.
 *
 * If the function returns `false`, the caller no longer owns the reference and
 * must not access the reference counted object.
 *
 * If the function returns `true`, the caller is now the sole owner of the
 * reference counted object, and must deallocate it.
 */
static inline bool refcount_down(atomic_refcount_t *rc)
{
	// NOTE: The decrementers don't need to synchronize with each other,
	//       but they do need to synchronize with the one doing deallocation.
	int old = atomic_fetch_sub_explicit(&rc->__cnt, 1,
	    memory_order_release);

	assert(old >= 0);

	if (old == 0) {
		// NOTE: We are holding the last reference, so we must now
		//       synchronize with all the other decrementers.

		int val = atomic_load_explicit(&rc->__cnt,
		    memory_order_acquire);
		assert(val == -1);

		/*
		 * The compiler probably wouldn't optimize the memory barrier
		 * away, but better safe than sorry.
		 */
		return val < 0;
	}

	return false;
}

#endif
