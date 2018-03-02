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

#include <stdbool.h>
#include <arch/barrier.h>
#include <assert.h>
#include <preemption.h>
#include <atomic.h>
#include <arch/asm.h>

#ifdef CONFIG_SMP

typedef struct spinlock {
	atomic_t val;

#ifdef CONFIG_DEBUG_SPINLOCK
	const char *name;
#endif /* CONFIG_DEBUG_SPINLOCK */
} spinlock_t;

/*
 * SPINLOCK_DECLARE is to be used for dynamically allocated spinlocks,
 * where the lock gets initialized in run time.
 */
#define SPINLOCK_DECLARE(lock_name)  spinlock_t lock_name
#define SPINLOCK_EXTERN(lock_name)   extern spinlock_t lock_name

/*
 * SPINLOCK_INITIALIZE and SPINLOCK_STATIC_INITIALIZE are to be used
 * for statically allocated spinlocks. They declare (either as global
 * or static) symbol and initialize the lock.
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

#define ASSERT_SPINLOCK(expr, lock) \
	assert_verbose(expr, (lock)->name)

#define spinlock_lock(lock)    spinlock_lock_debug((lock))
#define spinlock_unlock(lock)  spinlock_unlock_debug((lock))

#else /* CONFIG_DEBUG_SPINLOCK */

#define SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	spinlock_t lock_name = { \
		.val = { 0 } \
	}

#define SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static spinlock_t lock_name = { \
		.val = { 0 } \
	}

#define ASSERT_SPINLOCK(expr, lock) \
	assert(expr)

#define spinlock_lock(lock)    atomic_lock_arch(&(lock)->val)
#define spinlock_unlock(lock)  spinlock_unlock_nondebug((lock))

#endif /* CONFIG_DEBUG_SPINLOCK */

#define SPINLOCK_INITIALIZE(lock_name) \
	SPINLOCK_INITIALIZE_NAME(lock_name, #lock_name)

#define SPINLOCK_STATIC_INITIALIZE(lock_name) \
	SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, #lock_name)

extern void spinlock_initialize(spinlock_t *, const char *);
extern bool spinlock_trylock(spinlock_t *);
extern void spinlock_lock_debug(spinlock_t *);
extern void spinlock_unlock_debug(spinlock_t *);
extern bool spinlock_locked(spinlock_t *);

/** Unlock spinlock
 *
 * Unlock spinlock for non-debug kernels.
 *
 * @param sl Pointer to spinlock_t structure.
 *
 */
NO_TRACE static inline void spinlock_unlock_nondebug(spinlock_t *lock)
{
	/*
	 * Prevent critical section code from bleeding out this way down.
	 */
	CS_LEAVE_BARRIER();

	atomic_set(&lock->val, 0);
	preemption_enable();
}

#ifdef CONFIG_DEBUG_SPINLOCK

#include <log.h>

#define DEADLOCK_THRESHOLD  100000000

#define DEADLOCK_PROBE_INIT(pname)  size_t pname = 0

#define DEADLOCK_PROBE(pname, value) \
	if ((pname)++ > (value)) { \
		(pname) = 0; \
		log(LF_OTHER, LVL_WARN, \
		    "Deadlock probe %s: exceeded threshold %u\n" \
		    "cpu%u: function=%s, line=%u\n", \
		    #pname, (value), CPU->id, __func__, __LINE__); \
	}

#else /* CONFIG_DEBUG_SPINLOCK */

#define DEADLOCK_PROBE_INIT(pname)
#define DEADLOCK_PROBE(pname, value)

#endif /* CONFIG_DEBUG_SPINLOCK */

#else /* CONFIG_SMP */

/* On UP systems, spinlocks are effectively left out. */

/* Allow the use of spinlock_t as an incomplete type. */
typedef struct spinlock spinlock_t;

#define SPINLOCK_DECLARE(name)
#define SPINLOCK_EXTERN(name)

#define SPINLOCK_INITIALIZE(name)
#define SPINLOCK_STATIC_INITIALIZE(name)

#define SPINLOCK_INITIALIZE_NAME(name, desc_name)
#define SPINLOCK_STATIC_INITIALIZE_NAME(name, desc_name)

#define ASSERT_SPINLOCK(expr, lock)  assert(expr)

#define spinlock_initialize(lock, name)

#define spinlock_lock(lock)     preemption_disable()
#define spinlock_trylock(lock)  ({ preemption_disable(); 1; })
#define spinlock_unlock(lock)   preemption_enable()
#define spinlock_locked(lock)	1
#define spinlock_unlocked(lock)	1

#define DEADLOCK_PROBE_INIT(pname)
#define DEADLOCK_PROBE(pname, value)

#endif /* CONFIG_SMP */

typedef struct {
	SPINLOCK_DECLARE(lock);  /**< Spinlock */
	bool guard;              /**< Flag whether ipl is valid */
	ipl_t ipl;               /**< Original interrupt level */
} irq_spinlock_t;

#define IRQ_SPINLOCK_DECLARE(lock_name)  irq_spinlock_t lock_name
#define IRQ_SPINLOCK_EXTERN(lock_name)   extern irq_spinlock_t lock_name

#ifdef CONFIG_SMP

#define ASSERT_IRQ_SPINLOCK(expr, irq_lock) \
	ASSERT_SPINLOCK(expr, &((irq_lock)->lock))

/*
 * IRQ_SPINLOCK_INITIALIZE and IRQ_SPINLOCK_STATIC_INITIALIZE are to be used
 * for statically allocated interrupts-disabled spinlocks. They declare (either
 * as global or static symbol) and initialize the lock.
 */
#ifdef CONFIG_DEBUG_SPINLOCK

#define IRQ_SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	irq_spinlock_t lock_name = { \
		.lock = { \
			.name = desc_name, \
			.val = { 0 } \
		}, \
		.guard = false, \
		.ipl = 0 \
	}

#define IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static irq_spinlock_t lock_name = { \
		.lock = { \
			.name = desc_name, \
			.val = { 0 } \
		}, \
		.guard = false, \
		.ipl = 0 \
	}

#else /* CONFIG_DEBUG_SPINLOCK */

#define IRQ_SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	irq_spinlock_t lock_name = { \
		.lock = { \
			.val = { 0 } \
		}, \
		.guard = false, \
		.ipl = 0 \
	}

#define IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static irq_spinlock_t lock_name = { \
		.lock = { \
			.val = { 0 } \
		}, \
		.guard = false, \
		.ipl = 0 \
	}

#endif /* CONFIG_DEBUG_SPINLOCK */

#else /* CONFIG_SMP */

/*
 * Since the spinlocks are void on UP systems, we also need
 * to have a special variant of interrupts-disabled spinlock
 * macros which take this into account.
 */

#define ASSERT_IRQ_SPINLOCK(expr, irq_lock) \
	ASSERT_SPINLOCK(expr, NULL)

#define IRQ_SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	irq_spinlock_t lock_name = { \
		.guard = false, \
		.ipl = 0 \
	}

#define IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static irq_spinlock_t lock_name = { \
		.guard = false, \
		.ipl = 0 \
	}

#endif /* CONFIG_SMP */

#define IRQ_SPINLOCK_INITIALIZE(lock_name) \
	IRQ_SPINLOCK_INITIALIZE_NAME(lock_name, #lock_name)

#define IRQ_SPINLOCK_STATIC_INITIALIZE(lock_name) \
	IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, #lock_name)

extern void irq_spinlock_initialize(irq_spinlock_t *, const char *);
extern void irq_spinlock_lock(irq_spinlock_t *, bool);
extern void irq_spinlock_unlock(irq_spinlock_t *, bool);
extern bool irq_spinlock_trylock(irq_spinlock_t *);
extern void irq_spinlock_pass(irq_spinlock_t *, irq_spinlock_t *);
extern void irq_spinlock_exchange(irq_spinlock_t *, irq_spinlock_t *);
extern bool irq_spinlock_locked(irq_spinlock_t *);

#endif

/** @}
 */
