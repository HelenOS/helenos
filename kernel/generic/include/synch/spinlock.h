/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup sync
 * @{
 */
/** @file
 */

#ifndef KERN_SPINLOCK_H_
#define KERN_SPINLOCK_H_

#include <arch/types.h>
#include <preemption.h>
#include <atomic.h>
#include <debug.h>

#ifdef CONFIG_SMP
typedef struct {
#ifdef CONFIG_DEBUG_SPINLOCK
	char *name;
#endif
	atomic_t val;
} spinlock_t;

/*
 * SPINLOCK_DECLARE is to be used for dynamically allocated spinlocks,
 * where the lock gets initialized in run time.
 */
#define SPINLOCK_DECLARE(slname) 	spinlock_t slname

/*
 * SPINLOCK_INITIALIZE is to be used for statically allocated spinlocks.
 * It declares and initializes the lock.
 */
#ifdef CONFIG_DEBUG_SPINLOCK
#define SPINLOCK_INITIALIZE(slname) 	\
	spinlock_t slname = { 		\
		.name = #slname,	\
		.val = { 0 }		\
	}
#else
#define SPINLOCK_INITIALIZE(slname) 	\
	spinlock_t slname = { 		\
		.val = { 0 }		\
	}
#endif

extern void spinlock_initialize(spinlock_t *sl, char *name);
extern int spinlock_trylock(spinlock_t *sl);
extern void spinlock_lock_debug(spinlock_t *sl);

#ifdef CONFIG_DEBUG_SPINLOCK
#  define spinlock_lock(x) spinlock_lock_debug(x)
#else
#  define spinlock_lock(x) atomic_lock_arch(&(x)->val)
#endif

/** Unlock spinlock
 *
 * Unlock spinlock.
 *
 * @param sl Pointer to spinlock_t structure.
 */
static inline void spinlock_unlock(spinlock_t *sl)
{
	ASSERT(atomic_get(&sl->val) != 0);

	/*
	 * Prevent critical section code from bleeding out this way down.
	 */
	CS_LEAVE_BARRIER();
	
	atomic_set(&sl->val, 0);
	preemption_enable();
}

#else

typedef void spinlock_t;

/* On UP systems, spinlocks are effectively left out. */
#define SPINLOCK_DECLARE(name)
#define SPINLOCK_INITIALIZE(name)

#define spinlock_initialize(x, name)
#define spinlock_lock(x)		preemption_disable()
#define spinlock_trylock(x) 		(preemption_disable(), 1)
#define spinlock_unlock(x)		preemption_enable()

#endif

#endif

/** @}
 */
