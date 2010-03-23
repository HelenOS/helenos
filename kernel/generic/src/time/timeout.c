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

/** @addtogroup time
 * @{
 */

/**
 * @file
 * @brief		Timeout management functions.
 */

#include <time/timeout.h>
#include <typedefs.h>
#include <config.h>
#include <panic.h>
#include <synch/spinlock.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>

/** Initialize timeouts
 *
 * Initialize kernel timeouts.
 *
 */
void timeout_init(void)
{
	spinlock_initialize(&CPU->timeoutlock, "timeout_lock");
	list_initialize(&CPU->timeout_active_head);
}


/** Reinitialize timeout 
 *
 * Initialize all members except the lock.
 *
 * @param t		Timeout to be initialized.
 *
 */
void timeout_reinitialize(timeout_t *t)
{
	t->cpu = NULL;
	t->ticks = 0;
	t->handler = NULL;
	t->arg = NULL;
	link_initialize(&t->link);
}


/** Initialize timeout
 *
 * Initialize all members including the lock.
 *
 * @param t		Timeout to be initialized.
 *
 */
void timeout_initialize(timeout_t *t)
{
	spinlock_initialize(&t->lock, "timeout_t_lock");
	timeout_reinitialize(t);
}


/** Register timeout
 *
 * Insert timeout handler f (with argument arg)
 * to timeout list and make it execute in
 * time microseconds (or slightly more).
 *
 * @param t		Timeout structure.
 * @param time		Number of usec in the future to execute the handler.
 * @param f		Timeout handler function.
 * @param arg		Timeout handler argument.
 *
 */
void
timeout_register(timeout_t *t, uint64_t time, timeout_handler_t f, void *arg)
{
	timeout_t *hlp = NULL;
	link_t *l, *m;
	ipl_t ipl;
	uint64_t sum;

	ipl = interrupts_disable();
	spinlock_lock(&CPU->timeoutlock);
	spinlock_lock(&t->lock);

	if (t->cpu)
		panic("Unexpected: t->cpu != 0.");

	t->cpu = CPU;
	t->ticks = us2ticks(time);
	
	t->handler = f;
	t->arg = arg;

	/*
	 * Insert t into the active timeouts list according to t->ticks.
	 */
	sum = 0;
	l = CPU->timeout_active_head.next;
	while (l != &CPU->timeout_active_head) {
		hlp = list_get_instance(l, timeout_t, link);
		spinlock_lock(&hlp->lock);
		if (t->ticks < sum + hlp->ticks) {
			spinlock_unlock(&hlp->lock);
			break;
		}
		sum += hlp->ticks;
		spinlock_unlock(&hlp->lock);
		l = l->next;
	}

	m = l->prev;
	list_prepend(&t->link, m); /* avoid using l->prev */

	/*
	 * Adjust t->ticks according to ticks accumulated in h's predecessors.
	 */
	t->ticks -= sum;

	/*
	 * Decrease ticks of t's immediate succesor by t->ticks.
	 */
	if (l != &CPU->timeout_active_head) {
		spinlock_lock(&hlp->lock);
		hlp->ticks -= t->ticks;
		spinlock_unlock(&hlp->lock);
	}

	spinlock_unlock(&t->lock);
	spinlock_unlock(&CPU->timeoutlock);
	interrupts_restore(ipl);
}


/** Unregister timeout
 *
 * Remove timeout from timeout list.
 *
 * @param t		Timeout to unregister.
 *
 * @return		True on success, false on failure.
 */
bool timeout_unregister(timeout_t *t)
{
	timeout_t *hlp;
	link_t *l;
	ipl_t ipl;
	DEADLOCK_PROBE_INIT(p_tolock);

grab_locks:
	ipl = interrupts_disable();
	spinlock_lock(&t->lock);
	if (!t->cpu) {
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		return false;
	}
	if (!spinlock_trylock(&t->cpu->timeoutlock)) {
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		DEADLOCK_PROBE(p_tolock, DEADLOCK_THRESHOLD);
		goto grab_locks;
	}
	
	/*
	 * Now we know for sure that t hasn't been activated yet
	 * and is lurking in t->cpu->timeout_active_head queue.
	 */

	l = t->link.next;
	if (l != &t->cpu->timeout_active_head) {
		hlp = list_get_instance(l, timeout_t, link);
		spinlock_lock(&hlp->lock);
		hlp->ticks += t->ticks;
		spinlock_unlock(&hlp->lock);
	}
	
	list_remove(&t->link);
	spinlock_unlock(&t->cpu->timeoutlock);

	timeout_reinitialize(t);
	spinlock_unlock(&t->lock);

	interrupts_restore(ipl);
	return true;
}

/** @}
 */
