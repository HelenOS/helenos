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
	waitq_initialize_with_count(&sem->wq, val);
}

errno_t semaphore_trydown(semaphore_t *sem)
{
	return semaphore_down_timeout(sem, 0);
}

/** Semaphore down with timeout
 *
 * @param sem   Semaphore.
 * @param usec  Timeout in microseconds.
 *
 * @return See comment for waitq_sleep_timeout().
 *
 */
errno_t semaphore_down_timeout(semaphore_t *sem, uint32_t usec)
{
	errno_t rc = waitq_sleep_timeout(&sem->wq, usec);
	assert(rc == EOK || rc == ETIMEOUT || rc == EAGAIN);
	return rc;
}

void semaphore_down(semaphore_t *sem)
{
	errno_t rc = waitq_sleep(&sem->wq);
	assert(rc == EOK);
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
	waitq_wake_one(&sem->wq);
}

/** @}
 */
