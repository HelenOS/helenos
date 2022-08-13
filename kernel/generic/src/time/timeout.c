/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/** Reinitialize timeout
 *
 * Initialize all members except the lock.
 *
 * @param timeout Timeout to be initialized.
 *
 */
void timeout_reinitialize(timeout_t *timeout)
{
	timeout->cpu = NULL;
	timeout->ticks = 0;
	timeout->handler = NULL;
	timeout->arg = NULL;
	link_initialize(&timeout->link);
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
	irq_spinlock_initialize(&timeout->lock, "timeout_t_lock");
	timeout_reinitialize(timeout);
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
	irq_spinlock_lock(&timeout->lock, false);

	if (timeout->cpu)
		panic("Unexpected: timeout->cpu != 0.");

	timeout->cpu = CPU;
	timeout->ticks = us2ticks(time);

	timeout->handler = handler;
	timeout->arg = arg;

	/*
	 * Insert timeout into the active timeouts list according to timeout->ticks.
	 */
	uint64_t sum = 0;
	timeout_t *target = NULL;
	link_t *cur, *prev;
	prev = NULL;
	for (cur = list_first(&CPU->timeout_active_list);
	    cur != NULL; cur = list_next(cur, &CPU->timeout_active_list)) {
		target = list_get_instance(cur, timeout_t, link);
		irq_spinlock_lock(&target->lock, false);

		if (timeout->ticks < sum + target->ticks) {
			irq_spinlock_unlock(&target->lock, false);
			break;
		}

		sum += target->ticks;
		irq_spinlock_unlock(&target->lock, false);
		prev = cur;
	}

	if (prev == NULL)
		list_prepend(&timeout->link, &CPU->timeout_active_list);
	else
		list_insert_after(&timeout->link, prev);

	/*
	 * Adjust timeout->ticks according to ticks
	 * accumulated in target's predecessors.
	 */
	timeout->ticks -= sum;

	/*
	 * Decrease ticks of timeout's immediate succesor by timeout->ticks.
	 */
	if (cur != NULL) {
		irq_spinlock_lock(&target->lock, false);
		target->ticks -= timeout->ticks;
		irq_spinlock_unlock(&target->lock, false);
	}

	irq_spinlock_unlock(&timeout->lock, false);
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
	DEADLOCK_PROBE_INIT(p_tolock);

grab_locks:
	irq_spinlock_lock(&timeout->lock, true);
	if (!timeout->cpu) {
		irq_spinlock_unlock(&timeout->lock, true);
		return false;
	}

	if (!irq_spinlock_trylock(&timeout->cpu->timeoutlock)) {
		irq_spinlock_unlock(&timeout->lock, true);
		DEADLOCK_PROBE(p_tolock, DEADLOCK_THRESHOLD);
		goto grab_locks;
	}

	/*
	 * Now we know for sure that timeout hasn't been activated yet
	 * and is lurking in timeout->cpu->timeout_active_list.
	 */

	link_t *cur = list_next(&timeout->link,
	    &timeout->cpu->timeout_active_list);
	if (cur != NULL) {
		timeout_t *tmp = list_get_instance(cur, timeout_t, link);
		irq_spinlock_lock(&tmp->lock, false);
		tmp->ticks += timeout->ticks;
		irq_spinlock_unlock(&tmp->lock, false);
	}

	list_remove(&timeout->link);
	irq_spinlock_unlock(&timeout->cpu->timeoutlock, false);

	timeout_reinitialize(timeout);
	irq_spinlock_unlock(&timeout->lock, true);

	return true;
}

/** @}
 */
