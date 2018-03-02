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

#include <test.h>
#include <arch.h>
#include <atomic.h>
#include <print.h>
#include <proc/thread.h>
#include <typedefs.h>
#include <arch/context.h>

#include <synch/waitq.h>
#include <synch/semaphore.h>
#include <synch/spinlock.h>

static semaphore_t sem;

SPINLOCK_INITIALIZE(sem_lock);

static waitq_t can_start;

static uint32_t seed = 0xdeadbeef;

static uint32_t random(uint32_t max)
{
	uint32_t rc;

	spinlock_lock(&sem_lock);
	rc = seed % max;
	seed = (((seed << 2) ^ (seed >> 2)) * 487) + rc;
	spinlock_unlock(&sem_lock);
	return rc;
}

static void consumer(void *arg)
{
	errno_t rc;
	int to;

	thread_detach(THREAD);

	waitq_sleep(&can_start);

	to = random(20000);
	TPRINTF("cpu%u, tid %" PRIu64 " down+ (%d)\n", CPU->id, THREAD->tid, to);
	rc = semaphore_down_timeout(&sem, to);
	if (rc != EOK) {
		TPRINTF("cpu%u, tid %" PRIu64 " down!\n", CPU->id, THREAD->tid);
		return;
	}

	TPRINTF("cpu%u, tid %" PRIu64 " down=\n", CPU->id, THREAD->tid);
	thread_usleep(random(30000));

	semaphore_up(&sem);
	TPRINTF("cpu%u, tid %" PRIu64 " up\n", CPU->id, THREAD->tid);
}

const char *test_semaphore2(void)
{
	uint32_t i, k;

	waitq_initialize(&can_start);
	semaphore_initialize(&sem, 5);

	thread_t *thrd;

	k = random(7) + 1;
	TPRINTF("Creating %" PRIu32 " consumers\n", k);
	for (i = 0; i < k; i++) {
		thrd = thread_create(consumer, NULL, TASK,
		    THREAD_FLAG_NONE, "consumer");
		if (thrd)
			thread_ready(thrd);
		else
			TPRINTF("Error creating thread\n");
	}

	thread_usleep(20000);
	waitq_wakeup(&can_start, WAKEUP_ALL);

	return NULL;
}
