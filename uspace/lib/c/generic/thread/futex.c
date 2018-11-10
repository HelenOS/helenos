/*
 * Copyright (c) 2008 Jakub Jermar
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

#include <assert.h>
#include <stdatomic.h>
#include <fibril.h>
#include <io/kio.h>

#include "../private/fibril.h"
#include "../private/futex.h"

//#define DPRINTF(...) kio_printf(__VA_ARGS__)
#define DPRINTF(...) dummy_printf(__VA_ARGS__)

/** Initialize futex counter.
 *
 * @param futex Futex.
 * @param val   Initialization value.
 *
 * @return      Error code.
 */
errno_t futex_initialize(futex_t *futex, int val)
{
	atomic_store_explicit(&futex->val, val, memory_order_relaxed);
	futex->whandle = CAP_NIL;
	return futex_allocate_waitq(futex);
}

#ifdef CONFIG_DEBUG_FUTEX

void __futex_assert_is_locked(futex_t *futex, const char *name)
{
	void *owner = atomic_load_explicit(&futex->owner, memory_order_relaxed);
	fibril_t *self = (fibril_t *) fibril_get_id();
	if (owner != self) {
		DPRINTF("Assertion failed: %s (%p) is not locked by fibril %p (instead locked by fibril %p).\n", name, futex, self, owner);
	}
	assert(owner == self);
}

void __futex_assert_is_not_locked(futex_t *futex, const char *name)
{
	void *owner = atomic_load_explicit(&futex->owner, memory_order_relaxed);
	fibril_t *self = (fibril_t *) fibril_get_id();
	if (owner == self) {
		DPRINTF("Assertion failed: %s (%p) is already locked by fibril %p.\n", name, futex, self);
	}
	assert(owner != self);
}

void __futex_lock(futex_t *futex, const char *name)
{
	/*
	 * We use relaxed atomics to avoid violating C11 memory model.
	 * They should compile to regular load/stores, but simple assignments
	 * would be UB by definition.
	 * The proper ordering is ensured by the surrounding futex operation.
	 */

	fibril_t *self = (fibril_t *) fibril_get_id();
	DPRINTF("Locking futex %s (%p) by fibril %p.\n", name, futex, self);
	__futex_assert_is_not_locked(futex, name);
	futex_down(futex);

	void *prev_owner = atomic_load_explicit(&futex->owner,
	    memory_order_relaxed);
	assert(prev_owner == NULL);
	atomic_store_explicit(&futex->owner, self, memory_order_relaxed);
}

void __futex_unlock(futex_t *futex, const char *name)
{
	fibril_t *self = (fibril_t *) fibril_get_id();
	DPRINTF("Unlocking futex %s (%p) by fibril %p.\n", name, futex, self);
	__futex_assert_is_locked(futex, name);
	atomic_store_explicit(&futex->owner, NULL, memory_order_relaxed);
	futex_up(futex);
}

bool __futex_trylock(futex_t *futex, const char *name)
{
	fibril_t *self = (fibril_t *) fibril_get_id();
	bool success = futex_trydown(futex);
	if (success) {
		void *owner = atomic_load_explicit(&futex->owner,
		    memory_order_relaxed);
		assert(owner == NULL);

		atomic_store_explicit(&futex->owner, self, memory_order_relaxed);

		DPRINTF("Trylock on futex %s (%p) by fibril %p succeeded.\n", name, futex, self);
	} else {
		DPRINTF("Trylock on futex %s (%p) by fibril %p failed.\n", name, futex, self);
	}

	return success;
}

void __futex_give_to(futex_t *futex, void *new_owner, const char *name)
{
	fibril_t *self = fibril_self();
	fibril_t *no = new_owner;
	DPRINTF("Passing futex %s (%p) from fibril %p to fibril %p.\n", name, futex, self, no);

	__futex_assert_is_locked(futex, name);
	atomic_store_explicit(&futex->owner, new_owner, memory_order_relaxed);
}

#endif

/** @}
 */
