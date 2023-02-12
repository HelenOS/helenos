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
 * @brief Spinlocks.
 */

#ifdef CONFIG_SMP

#include <arch/asm.h>
#include <synch/spinlock.h>
#include <atomic.h>
#include <barrier.h>
#include <arch.h>
#include <preemption.h>
#include <stdio.h>
#include <debug.h>
#include <symtab.h>
#include <stacktrace.h>
#include <cpu.h>

/** Initialize spinlock
 *
 * @param sl Pointer to spinlock_t structure.
 *
 */
void spinlock_initialize(spinlock_t *lock, const char *name)
{
	atomic_flag_clear_explicit(&lock->flag, memory_order_relaxed);
#ifdef CONFIG_DEBUG_SPINLOCK
	lock->name = name;
#endif
}

/** Lock spinlock
 *
 * @param lock Pointer to spinlock_t structure.
 *
 */
void spinlock_lock(spinlock_t *lock)
{
	preemption_disable();

	bool deadlock_reported = false;
	size_t i = 0;

	while (atomic_flag_test_and_set_explicit(&lock->flag, memory_order_acquire)) {
		cpu_spin_hint();

#ifdef CONFIG_DEBUG_SPINLOCK
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
#endif
	}

	/* Avoid compiler warning with debug disabled. */
	(void) i;

	if (deadlock_reported)
		printf("cpu%u: not deadlocked\n", CPU->id);
}

/** Unlock spinlock
 *
 * @param sl Pointer to spinlock_t structure.
 */
void spinlock_unlock(spinlock_t *lock)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	ASSERT_SPINLOCK(spinlock_locked(lock), lock);
#endif

	atomic_flag_clear_explicit(&lock->flag, memory_order_release);
	preemption_enable();
}

/**
 * Lock spinlock conditionally. If the spinlock is not available
 * at the moment, signal failure.
 *
 * @param lock Pointer to spinlock_t structure.
 *
 * @return true on success.
 *
 */
bool spinlock_trylock(spinlock_t *lock)
{
	preemption_disable();

	bool ret = !atomic_flag_test_and_set_explicit(&lock->flag, memory_order_acquire);

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
	// NOTE: Atomic flag doesn't support simple atomic read (by design),
	//       so instead we test_and_set and then clear if necessary.
	//       This function is only used inside assert, so we don't need
	//       any preemption_disable/enable here.

	bool ret = atomic_flag_test_and_set_explicit(&lock->flag, memory_order_relaxed);
	if (!ret)
		atomic_flag_clear_explicit(&lock->flag, memory_order_relaxed);
	return ret;
}

#endif  /* CONFIG_SMP */

/** @}
 */
