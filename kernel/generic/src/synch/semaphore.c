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
