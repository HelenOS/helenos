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

#include <test.h>
#include <arch.h>
#include <atomic.h>
#include <print.h>
#include <proc/thread.h>
#include <arch/types.h>
#include <arch/context.h>

#include <synch/waitq.h>
#include <synch/semaphore.h>
#include <synch/synch.h>
#include <synch/spinlock.h>

static semaphore_t sem;

static SPINLOCK_INITIALIZE(lock);

static waitq_t can_start;

static uint32_t seed = 0xdeadbeef;

static uint32_t random(uint32_t max);

static void consumer(void *arg);
static void failed(void);

static uint32_t random(uint32_t max)
{
	uint32_t rc;

	spinlock_lock(&lock);	
	rc = seed % max;
	seed = (((seed<<2) ^ (seed>>2)) * 487) + rc;
	spinlock_unlock(&lock);
	return rc;
}


static void consumer(void *arg)
{
	int rc, to;
	
	thread_detach(THREAD);
	
	waitq_sleep(&can_start);
	
	to = random(20000);
	printf("cpu%d, tid %d down+ (%d)\n", CPU->id, THREAD->tid, to);
	rc = semaphore_down_timeout(&sem, to);
	if (SYNCH_FAILED(rc)) {
		printf("cpu%d, tid %d down!\n", CPU->id, THREAD->tid);
		return;
	}
	
	printf("cpu%d, tid %d down=\n", CPU->id, THREAD->tid);	
	thread_usleep(random(30000));
	
	semaphore_up(&sem);
	printf("cpu%d, tid %d up\n", CPU->id, THREAD->tid);
}

static void failed(void)
{
	printf("Test failed prematurely.\n");
	thread_exit();
}

void test_semaphore2(void)
{
	uint32_t i, k;
	
	printf("Semaphore test #2\n");
    
	waitq_initialize(&can_start);
	semaphore_initialize(&sem, 5);
	

	
	for (; ;) {
		thread_t *thrd;
		
		k = random(7) + 1;
		printf("Creating %d consumers\n", k);
		for (i=0; i<k; i++) {
			thrd = thread_create(consumer, NULL, TASK, 0, "consumer");
			if (thrd)
				thread_ready(thrd);
			else
				failed();
		}
		
		thread_usleep(20000);
		waitq_wakeup(&can_start, WAKEUP_ALL);
	}		

}
