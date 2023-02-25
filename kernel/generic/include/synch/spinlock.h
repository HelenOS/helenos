/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2023 Jiří Zárevúcky
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

/** @addtogroup kernel_sync
 * @{
 */
/** @file
 */

#ifndef KERN_SPINLOCK_H_
#define KERN_SPINLOCK_H_

#include <stdatomic.h>
#include <stdbool.h>

#include <arch/types.h>
#include <assert.h>

#define DEADLOCK_THRESHOLD  100000000

#if defined(CONFIG_SMP) && defined(CONFIG_DEBUG_SPINLOCK)

#include <log.h>

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

typedef struct spinlock {
#ifdef CONFIG_SMP
	atomic_flag flag;

#ifdef CONFIG_DEBUG_SPINLOCK
	const char *name;
#endif /* CONFIG_DEBUG_SPINLOCK */
#endif
} spinlock_t;

/*
 * SPINLOCK_DECLARE is to be used for dynamically allocated spinlocks,
 * where the lock gets initialized in run time.
 */
#define SPINLOCK_DECLARE(lock_name)  spinlock_t lock_name
#define SPINLOCK_EXTERN(lock_name)   extern spinlock_t lock_name

#ifdef CONFIG_SMP
#ifdef CONFIG_DEBUG_SPINLOCK
#define SPINLOCK_INITIALIZER(desc_name) { .name = (desc_name), .flag = ATOMIC_FLAG_INIT }
#else
#define SPINLOCK_INITIALIZER(desc_name) { .flag = ATOMIC_FLAG_INIT }
#endif
#else
#define SPINLOCK_INITIALIZER(desc_name) {}
#endif

/*
 * SPINLOCK_INITIALIZE and SPINLOCK_STATIC_INITIALIZE are to be used
 * for statically allocated spinlocks. They declare (either as global
 * or static) symbol and initialize the lock.
 */
#define SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	spinlock_t lock_name = SPINLOCK_INITIALIZER(desc_name)

#define SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static spinlock_t lock_name = SPINLOCK_INITIALIZER(desc_name)

#if defined(CONFIG_SMP) && defined(CONFIG_DEBUG_SPINLOCK)
#define ASSERT_SPINLOCK(expr, lock) assert_verbose(expr, (lock)->name)
#else /* CONFIG_DEBUG_SPINLOCK */
#define ASSERT_SPINLOCK(expr, lock) assert(expr)
#endif /* CONFIG_DEBUG_SPINLOCK */

#define SPINLOCK_INITIALIZE(lock_name) \
	SPINLOCK_INITIALIZE_NAME(lock_name, #lock_name)

#define SPINLOCK_STATIC_INITIALIZE(lock_name) \
	SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, #lock_name)

#ifdef CONFIG_SMP

extern void spinlock_initialize(spinlock_t *, const char *);
extern bool spinlock_trylock(spinlock_t *);
extern void spinlock_lock(spinlock_t *);
extern void spinlock_unlock(spinlock_t *);
extern bool spinlock_locked(spinlock_t *);

#else

#include <preemption.h>

static inline void spinlock_initialize(spinlock_t *l, const char *name)
{
}

static inline bool spinlock_trylock(spinlock_t *l)
{
	preemption_disable();
	return true;
}

static inline void spinlock_lock(spinlock_t *l)
{
	preemption_disable();
}

static inline void spinlock_unlock(spinlock_t *l)
{
	preemption_enable();
}

static inline bool spinlock_locked(spinlock_t *l)
{
	return true;
}

#endif

typedef struct {
	spinlock_t lock;              /**< Spinlock */
	bool guard;                   /**< Flag whether ipl is valid */
	ipl_t ipl;                    /**< Original interrupt level */
#ifdef CONFIG_DEBUG_SPINLOCK
	_Atomic(struct cpu *) owner;  /**< Which cpu currently owns this lock */
#endif
} irq_spinlock_t;

#define IRQ_SPINLOCK_DECLARE(lock_name)  irq_spinlock_t lock_name
#define IRQ_SPINLOCK_EXTERN(lock_name)   extern irq_spinlock_t lock_name

#define ASSERT_IRQ_SPINLOCK(expr, irq_lock) \
	ASSERT_SPINLOCK(expr, &((irq_lock)->lock))

#define IRQ_SPINLOCK_INITIALIZER(desc_name) \
	{ \
		.lock = SPINLOCK_INITIALIZER(desc_name), \
		.guard = false, \
		.ipl = 0, \
	}

/*
 * IRQ_SPINLOCK_INITIALIZE and IRQ_SPINLOCK_STATIC_INITIALIZE are to be used
 * for statically allocated interrupts-disabled spinlocks. They declare (either
 * as global or static symbol) and initialize the lock.
 */
#define IRQ_SPINLOCK_INITIALIZE_NAME(lock_name, desc_name) \
	irq_spinlock_t lock_name = IRQ_SPINLOCK_INITIALIZER(desc_name)

#define IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(lock_name, desc_name) \
	static irq_spinlock_t lock_name = IRQ_SPINLOCK_INITIALIZER(desc_name)

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
