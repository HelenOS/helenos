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

#include <time/clock.h>
#include <time/timeout.h>
#include <arch/types.h>
#include <config.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <func.h>
#include <proc/scheduler.h>
#include <cpu.h>
#include <print.h>
#include <arch.h>
#include <list.h>

#ifdef __SMP__
#include <arch/smp/atomic.h>
#endif

/*
 * Clock is called from an interrupt and is cpu_priority_high()'d.
 */
void clock(void)
{
	link_t *l;
	timeout_t *h;
	timeout_handler f;
	void *arg;

	/*
	 * To avoid lock ordering problems,
	 * run all expired timeouts as you visit them.
	 */
	spinlock_lock(&the->cpu->timeoutlock);
	while ((l = the->cpu->timeout_active_head.next) != &the->cpu->timeout_active_head) {
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
		spinlock_unlock(&the->cpu->timeoutlock);

		f(arg);

		spinlock_lock(&the->cpu->timeoutlock);
	}
	spinlock_unlock(&the->cpu->timeoutlock);

	/*
	 * Do CPU usage accounting and find out whether to preempt the->thread.
	 */

	if (the->thread) {
		spinlock_lock(&the->cpu->lock);
		the->cpu->needs_relink++;
		spinlock_unlock(&the->cpu->lock);	
	
		spinlock_lock(&the->thread->lock);
    		if (!the->thread->ticks--) {
			spinlock_unlock(&the->thread->lock);
			scheduler();
		}
		else {
			spinlock_unlock(&the->thread->lock);
		}
	}

}
