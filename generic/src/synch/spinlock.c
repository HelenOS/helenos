/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

/**
 * @file	spinlock.c
 * @brief	Spinlocks.
 */
 
#include <synch/spinlock.h>
#include <atomic.h>
#include <arch/barrier.h>
#include <arch.h>
#include <preemption.h>
#include <print.h>
#include <debug.h>
#include <symtab.h>

#ifdef CONFIG_FB
#include <genarch/fb/fb.h>
#endif

#ifdef CONFIG_SMP

/** Initialize spinlock
 *
 * Initialize spinlock.
 *
 * @param sl Pointer to spinlock_t structure.
 */
void spinlock_initialize(spinlock_t *sl, char *name)
{
	atomic_set(&sl->val, 0);
#ifdef CONFIG_DEBUG_SPINLOCK
	sl->name = name;
#endif	
}

/** Lock spinlock
 *
 * Lock spinlock.
 * This version has limitted ability to report 
 * possible occurence of deadlock.
 *
 * @param sl Pointer to spinlock_t structure.
 */
#ifdef CONFIG_DEBUG_SPINLOCK
void spinlock_lock_debug(spinlock_t *sl)
{
	count_t i = 0;
	char *symbol;
	bool deadlock_reported = false;

	preemption_disable();
	while (test_and_set(&sl->val)) {

		/*
		 * We need to be careful about printflock and fb_lock.
		 * Both of them are used to report deadlocks via
		 * printf() and fb_putchar().
		 *
		 * We trust our code that there is no possible deadlock
		 * caused by these two locks (except when an exception
		 * is triggered for instance by printf() or fb_putchar()).
		 * However, we encountered false positives caused by very
		 * slow VESA framebuffer interaction (especially when
		 * run in a simulator) that caused problems with both
		 * printflock and fb_lock.
		 *
		 * Possible deadlocks on both printflock and fb_lock
		 * are therefore not reported as they would cause an
		 * infinite recursion.
		 */
		if (sl == &printflock)
			continue;
#ifdef CONFIG_FB
		if (sl == &fb_lock)
			continue;
#endif
		if (i++ > 300000) {
			printf("cpu%d: looping on spinlock %.*p:%s, caller=%.*p",
			       CPU->id, sizeof(__address) * 2, sl, sl->name, sizeof(__address) * 2, CALLER);
			symbol = get_symtab_entry(CALLER);
			if (symbol)
				printf("(%s)", symbol);
			printf("\n");
			i = 0;
			deadlock_reported = true;
		}
	}

	if (deadlock_reported)
		printf("cpu%d: not deadlocked\n", CPU->id);

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
 * @param sl Pointer to spinlock_t structure.
 *
 * @return Zero on failure, non-zero otherwise.
 */
int spinlock_trylock(spinlock_t *sl)
{
	int rc;
	
	preemption_disable();
	rc = !test_and_set(&sl->val);

	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();

	if (!rc)
		preemption_enable();
	
	return rc;
}

#endif
