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

/**
 * @file
 * @brief Spinlocks.
 */

#include <synch/spinlock.h>
#include <atomic.h>
#include <arch/barrier.h>
#include <arch.h>
#include <preemption.h>
#include <print.h>
#include <debug.h>
#include <symtab.h>
#include <stacktrace.h>
#include <cpu.h>

#ifdef CONFIG_SMP

/** Initialize spinlock
 *
 * @param sl Pointer to spinlock_t structure.
 *
 */
void spinlock_initialize(spinlock_t *lock, const char *name)
{
	atomic_set(&lock->val, 0);
#ifdef CONFIG_DEBUG_SPINLOCK
	lock->name = name;
#endif
}

#ifdef CONFIG_DEBUG_SPINLOCK

/** Lock spinlock
 *
 * Lock spinlock.
 * This version has limitted ability to report
 * possible occurence of deadlock.
 *
 * @param lock Pointer to spinlock_t structure.
 *
 */
void spinlock_lock_debug(spinlock_t *lock)
{
	size_t i = 0;
	bool deadlock_reported = false;

	preemption_disable();
	while (test_and_set(&lock->val)) {
		/*
		 * We need to be careful about particular locks
		 * which are directly used to report deadlocks
		 * via printf() (and recursively other functions).
		 * This conserns especially printf_lock and the
		 * framebuffer lock.
		 *
		 * Any lock whose name is prefixed by "*" will be
		 * ignored by this deadlock detection routine
		 * as this might cause an infinite recursion.
		 * We trust our code that there is no possible deadlock
		 * caused by these locks (except when an exception
		 * is triggered for instance by printf()).
		 *
		 * We encountered false positives caused by very
		 * slow framebuffer interaction (especially when
		 * run in a simulator) that caused problems with both
		 * printf_lock and the framebuffer lock.
		 */
		if (lock->name[0] == '*')
			continue;

		if (i++ > DEADLOCK_THRESHOLD) {
			printf("cpu%u: looping on spinlock %p:%s, "
			    "caller=%p (%s)\n", CPU->id, lock, lock->name,
			    (void *) CALLER, symtab_fmt_name_lookup(CALLER));
			stack_trace();

			i = 0;
			deadlock_reported = true;
		}
	}

	if (deadlock_reported)
		printf("cpu%u: not deadlocked\n", CPU->id);

	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();
}

/** Unlock spinlock
 *
 * Unlock spinlock.
 *
 * @param sl Pointer to spinlock_t structure.
 */
void spinlock_unlock_debug(spinlock_t *lock)
{
	ASSERT_SPINLOCK(spinlock_locked(lock), lock);

	/*
	 * Prevent critical section code from bleeding out this way down.
	 */
	CS_LEAVE_BARRIER();

	atomic_set(&lock->val, 0);
	preemption_enable();
}

#endif

/** Lock spinlock conditionally
 *
 * Lock spinlock conditionally. If the spinlock is not available
 * at the moment, signal failure.
 *
 * @param lock Pointer to spinlock_t structure.
 *
 * @return Zero on failure, non-zero otherwise.
 *
 */
bool spinlock_trylock(spinlock_t *lock)
{
	preemption_disable();
	bool ret = !test_and_set(&lock->val);

	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();

	if (!ret)
		preemption_enable();

	return ret;
}

/** Find out whether the spinlock is currently locked.
 *
 * @param lock		Spinlock.
 * @return		True if the spinlock is locked, false otherwise.
 */
bool spinlock_locked(spinlock_t *lock)
{
	return atomic_get(&lock->val) != 0;
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
	spinlock_initialize(&(lock->lock), name);
	lock->guard = false;
	lock->ipl = 0;
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
void irq_spinlock_unlock(irq_spinlock_t *lock, bool irq_res)
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
bool irq_spinlock_trylock(irq_spinlock_t *lock)
{
	ASSERT_IRQ_SPINLOCK(interrupts_disabled(), lock);
	bool ret = spinlock_trylock(&(lock->lock));

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
void irq_spinlock_exchange(irq_spinlock_t *unlock, irq_spinlock_t *lock)
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

/** Find out whether the IRQ spinlock is currently locked.
 *
 * @param lock		IRQ spinlock.
 * @return		True if the IRQ spinlock is locked, false otherwise.
 */
bool irq_spinlock_locked(irq_spinlock_t *ilock)
{
	return spinlock_locked(&ilock->lock);
}

/** @}
 */
