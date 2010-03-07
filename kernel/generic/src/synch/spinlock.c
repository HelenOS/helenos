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
		 *
		 */
		if (lock->name[0] == '*')
			continue;
		
		if (i++ > DEADLOCK_THRESHOLD) {
			printf("cpu%u: looping on spinlock %" PRIp ":%s, "
			    "caller=%" PRIp "(%s)\n", CPU->id, lock, lock->name,
			    CALLER, symtab_fmt_name_lookup(CALLER));
			
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

#endif

/** Lock spinlock conditionally
 *
 * Lock spinlock conditionally.
 * If the spinlock is not available at the moment,
 * signal failure.
 *
 * @param lock Pointer to spinlock_t structure.
 *
 * @return Zero on failure, non-zero otherwise.
 *
 */
int spinlock_trylock(spinlock_t *lock)
{
	preemption_disable();
	int rc = !test_and_set(&lock->val);
	
	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();
	
	if (!rc)
		preemption_enable();
	
	return rc;
}

#endif

/** @}
 */
