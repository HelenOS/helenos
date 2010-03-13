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
#include <arch/barrier.h>
#include <preemption.h>
#include <atomic.h>
#include <debug.h>

#ifdef CONFIG_SMP

typedef struct {
	atomic_t val;
	
#ifdef CONFIG_DEBUG_SPINLOCK
	const char *name;
#endif
} spinlock_t;

/*
 * SPINLOCK_DECLARE is to be used for dynamically allocated spinlocks,
 * where the lock gets initialized in run time.
 */
#define SPINLOCK_DECLARE(lock_name)  spinlock_t lock_name
#define SPINLOCK_EXTERN(lock_name)   extern spinlock_t lock_name

/*
 * SPINLOCK_INITIALIZE is to be used for statically allocated spinlocks.
 * It declares and initializes the lock.
 */
#ifdef CONFIG_DEBUG_SPINLOCK

#define SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	spinlock_t lock_name = { \
		.name = desc_name, \
		.val = { 0 } \
	}

#define SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static spinlock_t lock_name = { \
		.name = desc_name, \
		.val = { 0 } \
	}

#define spinlock_lock(lock)  spinlock_lock_debug(lock)

#else

#define SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	spinlock_t lock_name = { \
		.val = { 0 } \
	}

#define SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static spinlock_t lock_name = { \
		.val = { 0 } \
	}

#define spinlock_lock(lock)  atomic_lock_arch(&(lock)->val)

#endif

#define SPINLOCK_INITIALIZE(lock_name) \
	SPINLOCK_INITIALIZE_NAME(lock_name, #lock_name)

#define SPINLOCK_STATIC_INITIALIZE(lock_name) \
	SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, #lock_name)

extern void spinlock_initialize(spinlock_t *lock, const char *name);
extern int spinlock_trylock(spinlock_t *lock);
extern void spinlock_lock_debug(spinlock_t *lock);

/** Unlock spinlock
 *
 * Unlock spinlock.
 *
 * @param sl Pointer to spinlock_t structure.
 */
static inline void spinlock_unlock(spinlock_t *lock)
{
	ASSERT(atomic_get(&lock->val) != 0);
	
	/*
	 * Prevent critical section code from bleeding out this way down.
	 */
	CS_LEAVE_BARRIER();
	
	atomic_set(&lock->val, 0);
	preemption_enable();
}

#ifdef CONFIG_DEBUG_SPINLOCK

#include <print.h>

#define DEADLOCK_THRESHOLD  100000000

#define DEADLOCK_PROBE_INIT(pname)  size_t pname = 0

#define DEADLOCK_PROBE(pname, value) \
	if ((pname)++ > (value)) { \
		(pname) = 0; \
		printf("Deadlock probe %s: exceeded threshold %u\n", \
		    "cpu%u: function=%s, line=%u\n", \
		    #pname, (value), CPU->id, __func__, __LINE__); \
	}

#else

#define DEADLOCK_PROBE_INIT(pname)
#define DEADLOCK_PROBE(pname, value)

#endif

#else /* CONFIG_SMP */

/* On UP systems, spinlocks are effectively left out. */

#define SPINLOCK_DECLARE(name)
#define SPINLOCK_EXTERN(name)

#define SPINLOCK_INITIALIZE(name)
#define SPINLOCK_STATIC_INITIALIZE(name)

#define SPINLOCK_INITIALIZE_NAME(name, desc_name)
#define SPINLOCK_STATIC_INITIALIZE_NAME(name, desc_name)

#define spinlock_initialize(lock, name)

#define spinlock_lock(lock)     preemption_disable()
#define spinlock_trylock(lock)  (preemption_disable(), 1)
#define spinlock_unlock(lock)   preemption_enable()

#define DEADLOCK_PROBE_INIT(pname)
#define DEADLOCK_PROBE(pname, value)

#endif

#endif

/** @}
 */
