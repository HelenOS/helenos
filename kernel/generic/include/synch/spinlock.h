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

#include <typedefs.h>
#include <arch/barrier.h>
#include <preemption.h>
#include <atomic.h>
#include <debug.h>

#ifdef CONFIG_SMP

typedef struct {
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
	ASSERT_VERBOSE(expr, (lock)->name)

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
	ASSERT(expr)

#define spinlock_lock(lock)    atomic_lock_arch(&(lock)->val)
#define spinlock_unlock(lock)  spinlock_unlock_nondebug((lock))

#endif /* CONFIG_DEBUG_SPINLOCK */

#define SPINLOCK_INITIALIZE(lock_name) \
	SPINLOCK_INITIALIZE_NAME(lock_name, #lock_name)

#define SPINLOCK_STATIC_INITIALIZE(lock_name) \
	SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, #lock_name)

extern void spinlock_initialize(spinlock_t *, const char *);
extern int spinlock_trylock(spinlock_t *);
extern void spinlock_lock_debug(spinlock_t *);
extern void spinlock_unlock_debug(spinlock_t *);

/** Unlock spinlock
 *
 * Unlock spinlock for non-debug kernels.
 *
 * @param sl Pointer to spinlock_t structure.
 *
 */
static inline void spinlock_unlock_nondebug(spinlock_t *lock)
{
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

#else /* CONFIG_DEBUG_SPINLOCK */

#define DEADLOCK_PROBE_INIT(pname)
#define DEADLOCK_PROBE(pname, value)

#endif /* CONFIG_DEBUG_SPINLOCK */

#else /* CONFIG_SMP */

/* On UP systems, spinlocks are effectively left out. */

#define SPINLOCK_DECLARE(name)
#define SPINLOCK_EXTERN(name)

#define SPINLOCK_INITIALIZE(name)
#define SPINLOCK_STATIC_INITIALIZE(name)

#define SPINLOCK_INITIALIZE_NAME(name, desc_name)
#define SPINLOCK_STATIC_INITIALIZE_NAME(name, desc_name)

#define ASSERT_SPINLOCK(expr, lock)

#define spinlock_initialize(lock, name)

#define spinlock_lock(lock)     preemption_disable()
#define spinlock_trylock(lock)  (preemption_disable(), 1)
#define spinlock_unlock(lock)   preemption_enable()

#define DEADLOCK_PROBE_INIT(pname)
#define DEADLOCK_PROBE(pname, value)

#endif /* CONFIG_SMP */

typedef struct {
	spinlock_t lock;  /**< Spinlock */
	bool guard;       /**< Flag whether ipl is valid */
	ipl_t ipl;        /**< Original interrupt level */
} irq_spinlock_t;

#define IRQ_SPINLOCK_DECLARE(lock_name)  irq_spinlock_t lock_name
#define IRQ_SPINLOCK_EXTERN(lock_name)   extern irq_spinlock_t lock_name

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

#define IRQ_SPINLOCK_INITIALIZE(lock_name) \
	IRQ_SPINLOCK_INITIALIZE_NAME(lock_name, #lock_name)

#define IRQ_SPINLOCK_STATIC_INITIALIZE(lock_name) \
	IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, #lock_name)

/** Initialize interrupts-disabled spinlock
 *
 * @param lock IRQ spinlock to be initialized.
 * @param name IRQ spinlock name.
 *
 */
static inline void irq_spinlock_initialize(irq_spinlock_t *lock, const char *name)
{
	spinlock_initialize(&(lock->lock), name);
	lock->guard = false;
	lock->ipl = 0;
}

/** Lock interrupts-disabled spinlock
 *
 * Lock a spinlock which requires disabled interrupts.
 *
 * @param lock    IRQ spinlock to be locked.
 * @param irq_dis If true, interrupts are actually disabled
 *                prior locking the spinlock. If false, interrupts
 *                are expected to be already disabled.
 *
 */
static inline void irq_spinlock_lock(irq_spinlock_t *lock, bool irq_dis)
{
	if (irq_dis) {
		ipl_t ipl = interrupts_disable();
		spinlock_lock(&(lock->lock));
		
		lock->guard = true;
		lock->ipl = ipl;
	} else {
		ASSERT_IRQ_SPINLOCK(interrupts_disabled(), lock);
		
		spinlock_lock(&(lock->lock));
		ASSERT_IRQ_SPINLOCK(!lock->guard, lock);
	}
}

/** Unlock interrupts-disabled spinlock
 *
 * Unlock a spinlock which requires disabled interrupts.
 *
 * @param lock    IRQ spinlock to be unlocked.
 * @param irq_res If true, interrupts are restored to previously
 *                saved interrupt level.
 *
 */
static inline void irq_spinlock_unlock(irq_spinlock_t *lock, bool irq_res)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), lock);
	
	if (irq_res) {
		ASSERT_IRQ_SPINLOCK(lock->guard, lock);
		
		lock->guard = false;
		ipl_t ipl = lock->ipl;
		
		spinlock_unlock(&(lock->lock));
		interrupts_restore(ipl);
	} else {
		ASSERT_IRQ_SPINLOCK(!lock->guard, lock);
		spinlock_unlock(&(lock->lock));
	}
}

/** Lock interrupts-disabled spinlock
 *
 * Lock an interrupts-disabled spinlock conditionally. If the
 * spinlock is not available at the moment, signal failure.
 * Interrupts are expected to be already disabled.
 *
 * @param lock IRQ spinlock to be locked conditionally.
 *
 * @return Zero on failure, non-zero otherwise.
 *
 */
static inline int irq_spinlock_trylock(irq_spinlock_t *lock)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), lock);
	int rc = spinlock_trylock(&(lock->lock));
	
	ASSERT_IRQ_SPINLOCK(!lock->guard, lock);
	return rc;
}

/** Pass lock from one interrupts-disabled spinlock to another
 *
 * Pass lock from one IRQ spinlock to another IRQ spinlock
 * without enabling interrupts during the process.
 *
 * The first IRQ spinlock is supposed to be locked.
 *
 * @param unlock IRQ spinlock to be unlocked.
 * @param lock   IRQ spinlock to be locked.
 *
 */
static inline void irq_spinlock_pass(irq_spinlock_t *unlock,
    irq_spinlock_t *lock)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), unlock);
	
	/* Pass guard from unlock to lock */
	bool guard = unlock->guard;
	ipl_t ipl = unlock->ipl;
	unlock->guard = false;
	
	spinlock_unlock(&(unlock->lock));
	spinlock_lock(&(lock->lock));
	
	ASSERT_IRQ_SPINLOCK(!lock->guard, lock);
	
	if (guard) {
		lock->guard = true;
		lock->ipl = ipl;
	}
}

/** Hand-over-hand locking of interrupts-disabled spinlocks
 *
 * Implement hand-over-hand locking between two interrupts-disabled
 * spinlocks without enabling interrupts during the process.
 *
 * The first IRQ spinlock is supposed to be locked.
 *
 * @param unlock IRQ spinlock to be unlocked.
 * @param lock   IRQ spinlock to be locked.
 *
 */
static inline void irq_spinlock_exchange(irq_spinlock_t *unlock,
    irq_spinlock_t *lock)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), unlock);
	
	spinlock_lock(&(lock->lock));
	ASSERT_IRQ_SPINLOCK(!lock->guard, lock);
	
	/* Pass guard from unlock to lock */
	if (unlock->guard) {
		lock->guard = true;
		lock->ipl = unlock->ipl;
		unlock->guard = false;
	}
	
	spinlock_unlock(&(unlock->lock));
}

#endif

/** @}
 */
