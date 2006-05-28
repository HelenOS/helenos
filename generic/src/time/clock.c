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
 * @file	clock.c
 * @brief	High-level clock interrupt handler.
 *
 * This file contains the clock() function which is the source
 * of preemption. It is also responsible for executing expired
 * timeouts.
 */
 
#include <time/clock.h>
#include <time/timeout.h>
#include <arch/types.h>
#include <config.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <func.h>
#include <proc/scheduler.h>
#include <cpu.h>
#include <arch.h>
#include <adt/list.h>
#include <atomic.h>
#include <proc/thread.h>

/** Clock routine
 *
 * Clock routine executed from clock interrupt handler
 * (assuming interrupts_disable()'d). Runs expired timeouts
 * and preemptive scheduling.
 *
 */
void clock(void)
{
	link_t *l;
	timeout_t *h;
	timeout_handler_t f;
	void *arg;
	int i;

	/*
	 * To avoid lock ordering problems,
	 * run all expired timeouts as you visit them.
	 */
	for (i = 0; i < CPU->missed_clock_ticks; i++) {
		spinlock_lock(&CPU->timeoutlock);
		while ((l = CPU->timeout_active_head.next) != &CPU->timeout_active_head) {
			h = list_get_instance(l, timeout_t, link);
			spinlock_lock(&h->lock);
			if (h->ticks-- != 0) {
				spinlock_unlock(&h->lock);
				break;
			}
			list_remove(l);
			f = h->handler;
			arg = h->arg;
			timeout_reinitialize(h);
			spinlock_unlock(&h->lock);	
			spinlock_unlock(&CPU->timeoutlock);

			f(arg);

			spinlock_lock(&CPU->timeoutlock);
		}
		spinlock_unlock(&CPU->timeoutlock);
	}
	CPU->missed_clock_ticks = 0;

	/*
	 * Do CPU usage accounting and find out whether to preempt THREAD.
	 */

	if (THREAD) {
		__u64 ticks;
		
		spinlock_lock(&CPU->lock);
		CPU->needs_relink++;
		spinlock_unlock(&CPU->lock);	
	
		spinlock_lock(&THREAD->lock);
		if ((ticks = THREAD->ticks))
			THREAD->ticks--;
		spinlock_unlock(&THREAD->lock);
		
		if (!ticks && !PREEMPTION_DISABLED) {
			scheduler();
		}
	}

}
