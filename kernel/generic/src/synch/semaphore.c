/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief Semaphores.
 */

#include <synch/semaphore.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <arch.h>

/** Initialize semaphore
 *
 * Initialize semaphore.
 *
 * @param sem Semaphore.
 * @param val Maximal number of threads allowed to enter critical section.
 *
 */
void semaphore_initialize(semaphore_t *sem, int val)
{
	waitq_initialize(&sem->wq);
	waitq_count_set(&sem->wq, val);
}

/** Semaphore down
 *
 * Semaphore down.
 * Conditional mode and mode with timeout can be requested.
 *
 * @param sem   Semaphore.
 * @param usec  Timeout in microseconds.
 * @param flags Select mode of operation.
 *
 * For exact description of possible combinations of
 * usec and flags, see comment for waitq_sleep_timeout().
 *
 * @return See comment for waitq_sleep_timeout().
 *
 */
errno_t _semaphore_down_timeout(semaphore_t *sem, uint32_t usec, unsigned int flags)
{
	return waitq_sleep_timeout(&sem->wq, usec, flags, NULL);
}

/** Semaphore up
 *
 * Semaphore up.
 *
 * @param s Semaphore.
 *
 */
void semaphore_up(semaphore_t *sem)
{
	waitq_wakeup(&sem->wq, WAKEUP_FIRST);
}

/** Get the semaphore counter value.
 *
 * @param sem		Semaphore.
 * @return		The number of threads that can down the semaphore
 * 			without blocking.
 */
int semaphore_count_get(semaphore_t *sem)
{
	return waitq_count_get(&sem->wq);
}

/** @}
 */
