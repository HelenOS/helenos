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

#include <synch/rwlock.h>

#define THREADS  4

static atomic_t thread_count;
static rwlock_t rwlock;

static void reader(void *arg)
{
	thread_detach(THREAD);
	
	TPRINTF("cpu%u, tid %" PRIu64 ": trying to lock rwlock for reading....\n", CPU->id, THREAD->tid);
	
	rwlock_read_lock(&rwlock);
	rwlock_read_unlock(&rwlock);
	
	TPRINTF("cpu%u, tid %" PRIu64 ": success\n", CPU->id, THREAD->tid);
	TPRINTF("cpu%u, tid %" PRIu64 ": trying to lock rwlock for writing....\n", CPU->id, THREAD->tid);
	
	rwlock_write_lock(&rwlock);
	rwlock_write_unlock(&rwlock);
	
	TPRINTF("cpu%u, tid %" PRIu64 ": success\n", CPU->id, THREAD->tid);
	
	atomic_dec(&thread_count);
}

const char *test_rwlock3(void)
{
	int i;
	thread_t *thrd;
	
	atomic_set(&thread_count, THREADS);
	
	rwlock_initialize(&rwlock);
	rwlock_write_lock(&rwlock);
	
	for (i = 0; i < THREADS; i++) {
		thrd = thread_create(reader, NULL, TASK, 0, "reader", false);
		if (thrd)
			thread_ready(thrd);
		else
			TPRINTF("Could not create reader %d\n", i);
	}
	
	thread_sleep(1);
	rwlock_write_unlock(&rwlock);
	
	while (atomic_get(&thread_count) > 0) {
		TPRINTF("Threads left: %ld\n", atomic_get(&thread_count));
		thread_sleep(1);
	}
	
	return NULL;
}
