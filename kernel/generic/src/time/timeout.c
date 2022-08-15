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

/** @addtogroup kernel_time
 * @{
 */

/**
 * @file
 * @brief Timeout management functions.
 */

#include <time/timeout.h>
#include <typedefs.h>
#include <config.h>
#include <panic.h>
#include <synch/spinlock.h>
#include <halt.h>
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
	irq_spinlock_initialize(&CPU->timeoutlock, "cpu.timeoutlock");
	list_initialize(&CPU->timeout_active_list);
}

/** Initialize timeout
 *
 * Initialize all members including the lock.
 *
 * @param timeout Timeout to be initialized.
 *
 */
void timeout_initialize(timeout_t *timeout)
{
	link_initialize(&timeout->link);
	timeout->cpu = NULL;
}

/** Register timeout
 *
 * Insert timeout handler f (with argument arg)
 * to timeout list and make it execute in
 * time microseconds (or slightly more).
 *
 * @param timeout Timeout structure.
 * @param time    Number of usec in the future to execute the handler.
 * @param handler Timeout handler function.
 * @param arg     Timeout handler argument.
 *
 */
void timeout_register(timeout_t *timeout, uint64_t time,
    timeout_handler_t handler, void *arg)
{
	irq_spinlock_lock(&CPU->timeoutlock, true);

	assert(!link_in_use(&timeout->link));

	timeout->cpu = CPU;
	timeout->deadline = CPU->current_clock_tick + us2ticks(time);
	timeout->handler = handler;
	timeout->arg = arg;

	/* Insert timeout into the active timeouts list according to timeout->deadline. */

	link_t *last = list_last(&CPU->timeout_active_list);
	if (last == NULL || timeout->deadline >= list_get_instance(last, timeout_t, link)->deadline) {
		list_append(&timeout->link, &CPU->timeout_active_list);
	} else {
		for (link_t *cur = list_first(&CPU->timeout_active_list); cur != NULL;
		    cur = list_next(cur, &CPU->timeout_active_list)) {

			if (timeout->deadline < list_get_instance(cur, timeout_t, link)->deadline) {
				list_insert_before(&timeout->link, cur);
				break;
			}
		}
	}

	irq_spinlock_unlock(&CPU->timeoutlock, true);
}

/** Unregister timeout
 *
 * Remove timeout from timeout list.
 *
 * @param timeout Timeout to unregister.
 *
 * @return True on success, false on failure.
 *
 */
bool timeout_unregister(timeout_t *timeout)
{
	assert(timeout->cpu);

	irq_spinlock_lock(&timeout->cpu->timeoutlock, true);

	bool success = link_in_use(&timeout->link);
	if (success) {
		list_remove(&timeout->link);
	}

	irq_spinlock_unlock(&timeout->cpu->timeoutlock, true);
	return success;
}

/** @}
 */
