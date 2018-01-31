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

#include <atomic.h>
#include <errno.h>
#include <libc.h>

typedef struct futex {
	atomic_t val;
#ifdef FUTEX_UPGRADABLE
	int upgraded;
#endif
} futex_t;


extern void futex_initialize(futex_t *futex, int value);

#ifdef FUTEX_UPGRADABLE
#include <rcu.h>

#define FUTEX_INITIALIZE(val) {{ (val) }, 0}

#define futex_lock(fut) \
({ \
	rcu_read_lock(); \
	(fut)->upgraded = rcu_access(_upgrade_futexes); \
	if ((fut)->upgraded) \
		(void) futex_down((fut)); \
})

#define futex_trylock(fut) \
({ \
	rcu_read_lock(); \
	int _upgraded = rcu_access(_upgrade_futexes); \
	if (_upgraded) { \
		int _acquired = futex_trydown((fut)); \
		if (!_acquired) { \
			rcu_read_unlock(); \
		} else { \
			(fut)->upgraded = true; \
		} \
		_acquired; \
	} else { \
		(fut)->upgraded = false; \
		1; \
	} \
})
		
#define futex_unlock(fut) \
({ \
	if ((fut)->upgraded) \
		(void) futex_up((fut)); \
	rcu_read_unlock(); \
})

extern int _upgrade_futexes;

extern void futex_upgrade_all_and_wait(void);
		
#else

#define FUTEX_INITIALIZE(val) {{ (val) }}

#define futex_lock(fut)     (void) futex_down((fut))
#define futex_trylock(fut)  futex_trydown((fut))
#define futex_unlock(fut)   (void) futex_up((fut))
		
#endif

#define FUTEX_INITIALIZER     FUTEX_INITIALIZE(1)

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
	return cas(&futex->val, 1, 0);
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
	if ((atomic_signed_t) atomic_predec(&futex->val) < 0)
		return (errno_t) __SYSCALL1(SYS_FUTEX_SLEEP, (sysarg_t) &futex->val.count);
	
	return EOK;
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
		return (errno_t) __SYSCALL1(SYS_FUTEX_WAKEUP, (sysarg_t) &futex->val.count);
	
	return EOK;
}

#endif

/** @}
 */
