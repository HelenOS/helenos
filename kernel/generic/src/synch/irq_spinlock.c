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

/**
 * @file
 * @brief IRQ Spinlocks.
 */

#include <arch/asm.h>
#include <synch/spinlock.h>

#include <cpu.h>

#ifdef CONFIG_DEBUG_SPINLOCK

#define CPU_OWNER ((CPU == NULL) ? (cpu_t *) UINTPTR_MAX : CPU)

static inline bool owned_by_me(irq_spinlock_t *lock)
{
	return atomic_load_explicit(&lock->owner, memory_order_relaxed) == CPU_OWNER;
}

static inline bool not_owned_by_me(irq_spinlock_t *lock)
{
	return !owned_by_me(lock);
}

static inline void claim(irq_spinlock_t *lock)
{
	cpu_t *cpu = CPU_OWNER;
	atomic_store_explicit(&lock->owner, cpu, memory_order_relaxed);
	CURRENT->mutex_locks++;
}

static inline void unclaim(irq_spinlock_t *lock)
{
	CURRENT->mutex_locks--;
	atomic_store_explicit(&lock->owner, NULL, memory_order_relaxed);
}

#else

static inline bool owned_by_me(irq_spinlock_t *lock)
{
	return true;
}

static inline bool not_owned_by_me(irq_spinlock_t *lock)
{
	return true;
}

static inline void claim(irq_spinlock_t *lock)
{
}

static inline void unclaim(irq_spinlock_t *lock)
{
}

#endif

/** Initialize interrupts-disabled spinlock
 *
 * @param lock IRQ spinlock to be initialized.
 * @param name IRQ spinlock name.
 *
 */
void irq_spinlock_initialize(irq_spinlock_t *lock, const char *name)
{
	*lock = (irq_spinlock_t) IRQ_SPINLOCK_INITIALIZER(name);
}

/** Lock interrupts-disabled spinlock
 *
 * Lock a spinlock which requires disabled interrupts.
 *
 * @param lock    IRQ spinlock to be locked.
 * @param irq_dis If true, disables interrupts before locking the spinlock.
 *                If false, interrupts are expected to be already disabled.
 *
 */
void irq_spinlock_lock(irq_spinlock_t *lock, bool irq_dis)
{
	ASSERT_IRQ_SPINLOCK(not_owned_by_me(lock), lock);

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

	claim(lock);
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
void irq_spinlock_unlock(irq_spinlock_t *lock, bool irq_res)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), lock);
	ASSERT_IRQ_SPINLOCK(owned_by_me(lock), lock);

	unclaim(lock);

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
bool irq_spinlock_trylock(irq_spinlock_t *lock)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), lock);
	bool ret = spinlock_trylock(&(lock->lock));
	if (ret)
		claim(lock);

	ASSERT_IRQ_SPINLOCK((!ret) || (!lock->guard), lock);
	return ret;
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
void irq_spinlock_pass(irq_spinlock_t *unlock, irq_spinlock_t *lock)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), unlock);
	ASSERT_IRQ_SPINLOCK(owned_by_me(unlock), unlock);
	ASSERT_IRQ_SPINLOCK(not_owned_by_me(lock), lock);

	/* Pass guard from unlock to lock */
	bool guard = unlock->guard;
	ipl_t ipl = unlock->ipl;
	unlock->guard = false;

	unclaim(unlock);

	spinlock_unlock(&(unlock->lock));
	spinlock_lock(&(lock->lock));

	claim(lock);

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
void irq_spinlock_exchange(irq_spinlock_t *unlock, irq_spinlock_t *lock)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), unlock);
	ASSERT_IRQ_SPINLOCK(owned_by_me(unlock), unlock);
	ASSERT_IRQ_SPINLOCK(not_owned_by_me(lock), lock);

	spinlock_lock(&(lock->lock));
	ASSERT_IRQ_SPINLOCK(!lock->guard, lock);

	/* Pass guard from unlock to lock */
	if (unlock->guard) {
		lock->guard = true;
		lock->ipl = unlock->ipl;
		unlock->guard = false;
	}

	claim(lock);
	unclaim(unlock);

	spinlock_unlock(&(unlock->lock));
}

/** Find out whether the IRQ spinlock is currently locked.
 *
 * @param lock		IRQ spinlock.
 * @return		True if the IRQ spinlock is locked, false otherwise.
 */
bool irq_spinlock_locked(irq_spinlock_t *lock)
{
	return owned_by_me(lock) && spinlock_locked(&lock->lock);
}

/** @}
 */
