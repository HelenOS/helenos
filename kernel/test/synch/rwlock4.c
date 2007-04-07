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
#include <arch/types.h>
#include <arch/context.h>
#include <context.h>

#include <synch/waitq.h>
#include <synch/rwlock.h>
#include <synch/synch.h>
#include <synch/spinlock.h>

#define READERS		50
#define WRITERS		50

static atomic_t thread_count;
static rwlock_t rwlock;
static atomic_t threads_fault;
static bool sh_quiet;

SPINLOCK_INITIALIZE(rw_lock);

static waitq_t can_start;

static uint32_t seed = 0xdeadbeef;

static uint32_t random(uint32_t max)
{
	uint32_t rc;

	spinlock_lock(&rw_lock);	
	rc = seed % max;
	seed = (((seed<<2) ^ (seed>>2)) * 487) + rc;
	spinlock_unlock(&rw_lock);
	return rc;
}

static void writer(void *arg)
{
	int rc, to;
	thread_detach(THREAD);
	waitq_sleep(&can_start);

	to = random(40000);
	
	if (!sh_quiet)
		printf("cpu%d, tid %llu w+ (%d)\n", CPU->id, THREAD->tid, to);
	
	rc = rwlock_write_lock_timeout(&rwlock, to);
	if (SYNCH_FAILED(rc)) {
		if (!sh_quiet)
			printf("cpu%d, tid %llu w!\n", CPU->id, THREAD->tid);
		atomic_dec(&thread_count);
		return;
	}
	
	if (!sh_quiet)
		printf("cpu%d, tid %llu w=\n", CPU->id, THREAD->tid);

	if (rwlock.readers_in) {
		if (!sh_quiet)
			printf("Oops.");
		atomic_inc(&threads_fault);
		atomic_dec(&thread_count);
		return;
	}
	thread_usleep(random(1000000));
	if (rwlock.readers_in) {
		if (!sh_quiet)
			printf("Oops.");	
		atomic_inc(&threads_fault);
		atomic_dec(&thread_count);
		return;
	}

	rwlock_write_unlock(&rwlock);
	
	if (!sh_quiet)
		printf("cpu%d, tid %llu w-\n", CPU->id, THREAD->tid);
	atomic_dec(&thread_count);
}

static void reader(void *arg)
{
	int rc, to;
	thread_detach(THREAD);
	waitq_sleep(&can_start);
	
	to = random(2000);
	
	if (!sh_quiet)
		printf("cpu%d, tid %llu r+ (%d)\n", CPU->id, THREAD->tid, to);
	
	rc = rwlock_read_lock_timeout(&rwlock, to);
	if (SYNCH_FAILED(rc)) {
		if (!sh_quiet)
			printf("cpu%d, tid %llu r!\n", CPU->id, THREAD->tid);
		atomic_dec(&thread_count);
		return;
	}
	
	if (!sh_quiet)
		printf("cpu%d, tid %llu r=\n", CPU->id, THREAD->tid);
	
	thread_usleep(30000);
	rwlock_read_unlock(&rwlock);
	
	if (!sh_quiet)
		printf("cpu%d, tid %llu r-\n", CPU->id, THREAD->tid);
	atomic_dec(&thread_count);
}

char * test_rwlock4(bool quiet)
{
	context_t ctx;
	uint32_t i;
	sh_quiet = quiet;
	
	waitq_initialize(&can_start);
	rwlock_initialize(&rwlock);
	atomic_set(&threads_fault, 0);
	
	uint32_t rd = random(7) + 1;
	uint32_t wr = random(5) + 1;
	
	atomic_set(&thread_count, rd + wr);
	
	thread_t *thrd;
	
	context_save(&ctx);
	if (!quiet) {
		printf("sp=%#x, readers_in=%d\n", ctx.sp, rwlock.readers_in);
		printf("Creating %d readers\n", rd);
	}
	
	for (i = 0; i < rd; i++) {
		thrd = thread_create(reader, NULL, TASK, 0, "reader", false);
		if (thrd)
			thread_ready(thrd);
		else if (!quiet)
			printf("Could not create reader %d\n", i);
	}

	if (!quiet)
		printf("Creating %d writers\n", wr);
	
	for (i = 0; i < wr; i++) {
		thrd = thread_create(writer, NULL, TASK, 0, "writer", false);
		if (thrd)
			thread_ready(thrd);
		else if (!quiet)
			printf("Could not create writer %d\n", i);
	}
	
	thread_usleep(20000);
	waitq_wakeup(&can_start, WAKEUP_ALL);
	
	while (atomic_get(&thread_count) > 0) {
		if (!quiet)
			printf("Threads left: %d\n", atomic_get(&thread_count));
		thread_sleep(1);
	}
	
	if (atomic_get(&threads_fault) == 0)
		return NULL;
	
	return "Test failed";
}
