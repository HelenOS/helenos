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

#ifndef LIBC_FUTEX_H_
#define LIBC_FUTEX_H_

#include <assert.h>
#include <atomic.h>
#include <errno.h>
#include <libc.h>
#include <time.h>

typedef struct futex {
	atomic_t val;
#ifdef CONFIG_DEBUG_FUTEX
	void *owner;
#endif
} futex_t;

extern void futex_initialize(futex_t *futex, int value);

#ifdef CONFIG_DEBUG_FUTEX

#define FUTEX_INITIALIZE(val) {{ (val) }, NULL }
#define FUTEX_INITIALIZER     FUTEX_INITIALIZE(1)

void __futex_assert_is_locked(futex_t *, const char *);
void __futex_assert_is_not_locked(futex_t *, const char *);
void __futex_lock(futex_t *, const char *);
void __futex_unlock(futex_t *, const char *);
bool __futex_trylock(futex_t *, const char *);
void __futex_give_to(futex_t *, void *, const char *);

#define futex_lock(futex) __futex_lock((futex), #futex)
#define futex_unlock(futex) __futex_unlock((futex), #futex)
#define futex_trylock(futex) __futex_trylock((futex), #futex)

#define futex_give_to(futex, new_owner) __futex_give_to((futex), (new_owner), #futex)
#define futex_assert_is_locked(futex) __futex_assert_is_locked((futex), #futex)
#define futex_assert_is_not_locked(futex) __futex_assert_is_not_locked((futex), #futex)

#else

#define FUTEX_INITIALIZE(val) {{ (val) }}
#define FUTEX_INITIALIZER     FUTEX_INITIALIZE(1)

#define futex_lock(fut)     (void) futex_down((fut))
#define futex_trylock(fut)  futex_trydown((fut))
#define futex_unlock(fut)   (void) futex_up((fut))

#define futex_give_to(fut, owner) ((void)0)
#define futex_assert_is_locked(fut) assert((atomic_signed_t) (fut)->val.count <= 0)
#define futex_assert_is_not_locked(fut) ((void)0)

#endif

/** Down the futex with timeout, composably.
 *
 * This means that when the operation fails due to a timeout or being
 * interrupted, the next futex_up() is ignored, which allows certain kinds of
 * composition of synchronization primitives.
 *
 * In most other circumstances, regular futex_down_timeout() is a better choice.
 *
 * @param futex Futex.
 *
 * @return ENOENT if there is no such virtual address.
 * @return ETIMEOUT if timeout expires.
 * @return EOK on success.
 * @return Error code from <errno.h> otherwise.
 *
 */
static inline errno_t futex_down_composable(futex_t *futex,
    const struct timespec *expires)
{
	// TODO: Add tests for this.

	if ((atomic_signed_t) atomic_predec(&futex->val) >= 0)
		return EOK;

	usec_t timeout;

	if (!expires) {
		/* No timeout. */
		timeout = 0;
	} else {
		if (expires->tv_sec == 0) {
			/* We can't just return ETIMEOUT. That wouldn't be composable. */
			timeout = 1;
		} else {
			struct timespec tv;
			getuptime(&tv);
			timeout = ts_gteq(&tv, expires) ? 1 :
			    NSEC2USEC(ts_sub_diff(expires, &tv));
		}

		assert(timeout > 0);
	}

	return __SYSCALL2(SYS_FUTEX_SLEEP, (sysarg_t) &futex->val.count, (sysarg_t) timeout);
}

/** Up the futex.
 *
 * @param futex Futex.
 *
 * @return ENOENT if there is no such virtual address.
 * @return EOK on success.
 * @return Error code from <errno.h> otherwise.
 *
 */
static inline errno_t futex_up(futex_t *futex)
{
	if ((atomic_signed_t) atomic_postinc(&futex->val) < 0)
		return __SYSCALL1(SYS_FUTEX_WAKEUP, (sysarg_t) &futex->val.count);

	return EOK;
}

static inline errno_t futex_down_timeout(futex_t *futex,
    const struct timespec *expires)
{
	if (expires && expires->tv_sec == 0 && expires->tv_nsec == 0) {
		/* Nonblocking down. */

		/*
		 * Try good old CAS a few times.
		 * Not too much though, we don't want to bloat the caller.
		 */
		for (int i = 0; i < 2; i++) {
			atomic_signed_t old = atomic_get(&futex->val);
			if (old <= 0)
				return ETIMEOUT;

			if (cas(&futex->val, old, old - 1))
				return EOK;
		}

		// TODO: builtin atomics with relaxed ordering can make this
		//       faster.

		/*
		 * If we don't succeed with CAS, we can't just return failure
		 * because that would lead to spurious failures where
		 * futex_down_timeout returns ETIMEOUT despite there being
		 * available tokens. That could break some algorithms.
		 * We also don't want to loop on CAS indefinitely, because
		 * that would make the semaphore not wait-free, even when all
		 * atomic operations and the underlying base semaphore are all
		 * wait-free.
		 * Instead, we fall back to regular down_timeout(), with
		 * an already expired deadline. That way we delegate all these
		 * concerns to the base semaphore.
		 */
	}

	/*
	 * This combination of a "composable" sleep followed by futex_up() on
	 * failure is necessary to prevent breakage due to certain race
	 * conditions.
	 */
	errno_t rc = futex_down_composable(futex, expires);
	if (rc != EOK)
		futex_up(futex);
	return rc;
}

/** Try to down the futex.
 *
 * @param futex Futex.
 *
 * @return true if the futex was acquired.
 * @return false if the futex was not acquired.
 *
 */
static inline bool futex_trydown(futex_t *futex)
{
	/*
	 * down_timeout with an already expired deadline should behave like
	 * trydown.
	 */
	struct timespec tv = { .tv_sec = 0, .tv_nsec = 0 };
	return futex_down_timeout(futex, &tv) == EOK;
}

/** Down the futex.
 *
 * @param futex Futex.
 *
 * @return ENOENT if there is no such virtual address.
 * @return EOK on success.
 * @return Error code from <errno.h> otherwise.
 *
 */
static inline errno_t futex_down(futex_t *futex)
{
	return futex_down_timeout(futex, NULL);
}

#endif

/** @}
 */
