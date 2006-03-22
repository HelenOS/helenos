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
#include <context.h>
#include <panic.h>

#include <synch/waitq.h>
#include <synch/rwlock.h>
#include <synch/synch.h>
#include <synch/spinlock.h>

#define READERS		50
#define WRITERS		50

static rwlock_t rwlock;

SPINLOCK_INITIALIZE(lock);

static waitq_t can_start;

__u32 seed = 0xdeadbeef;

static __u32 random(__u32 max);

static void writer(void *arg);
static void reader(void *arg);
static void failed(void);

__u32 random(__u32 max)
{
	__u32 rc;

	spinlock_lock(&lock);	
	rc = seed % max;
	seed = (((seed<<2) ^ (seed>>2)) * 487) + rc;
	spinlock_unlock(&lock);
	return rc;
}


void writer(void *arg)
{
	int rc, to;
	waitq_sleep(&can_start);

	to = random(40000);
	printf("cpu%d, tid %d w+ (%d)\n", CPU->id, THREAD->tid, to);
	rc = rwlock_write_lock_timeout(&rwlock, to);
	if (SYNCH_FAILED(rc)) {
		printf("cpu%d, tid %d w!\n", CPU->id, THREAD->tid);
		return;
	};
	printf("cpu%d, tid %d w=\n", CPU->id, THREAD->tid);

	if (rwlock.readers_in) panic("Oops.");
	thread_usleep(random(1000000));
	if (rwlock.readers_in) panic("Oops.");	

	rwlock_write_unlock(&rwlock);
	printf("cpu%d, tid %d w-\n", CPU->id, THREAD->tid);	
}

void reader(void *arg)
{
	int rc, to;
	waitq_sleep(&can_start);
	
	to = random(2000);
	printf("cpu%d, tid %d r+ (%d)\n", CPU->id, THREAD->tid, to);
	rc = rwlock_read_lock_timeout(&rwlock, to);
	if (SYNCH_FAILED(rc)) {
		printf("cpu%d, tid %d r!\n", CPU->id, THREAD->tid);
		return;
	}
	printf("cpu%d, tid %d r=\n", CPU->id, THREAD->tid);
	thread_usleep(30000);
	rwlock_read_unlock(&rwlock);
	printf("cpu%d, tid %d r-\n", CPU->id, THREAD->tid);		
}

void failed(void)
{
	printf("Test failed prematurely.\n");
	thread_exit();
}

void test(void)
{
	context_t ctx;
	__u32 i, k;
	
	printf("Read/write locks test #4\n");
    
	waitq_initialize(&can_start);
	rwlock_initialize(&rwlock);
	

	
	for (; ;) {
		thread_t *thrd;
		
		context_save(&ctx);
		printf("sp=%X, readers_in=%d\n", ctx.sp, rwlock.readers_in);
		
		k = random(7) + 1;
		printf("Creating %d readers\n", k);
		for (i=0; i<k; i++) {
			thrd = thread_create(reader, NULL, TASK, 0, "reader");
			if (thrd)
				thread_ready(thrd);
			else
				failed();
		}

		k = random(5) + 1;
		printf("Creating %d writers\n", k);
		for (i=0; i<k; i++) {
			thrd = thread_create(writer, NULL, TASK, 0, "writer");
			if (thrd)
				thread_ready(thrd);
			else
				failed();
		}
		
		thread_usleep(20000);
		waitq_wakeup(&can_start, WAKEUP_ALL);
	}		

}
