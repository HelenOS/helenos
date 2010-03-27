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

#include <synch/waitq.h>
#include <synch/rwlock.h>

#define READERS  50
#define WRITERS  50

static rwlock_t rwlock;

static waitq_t can_start;
static atomic_t items_read;
static atomic_t items_written;

static void writer(void *arg)
{
	thread_detach(THREAD);
	
	waitq_sleep(&can_start);
	
	rwlock_write_lock(&rwlock);
	atomic_inc(&items_written);
	rwlock_write_unlock(&rwlock);
}

static void reader(void *arg)
{
	thread_detach(THREAD);
	
	waitq_sleep(&can_start);
	
	rwlock_read_lock(&rwlock);
	atomic_inc(&items_read);
	rwlock_read_unlock(&rwlock);
}

const char *test_rwlock5(void)
{
	int i, j, k;
	atomic_count_t readers;
	atomic_count_t writers;
	
	waitq_initialize(&can_start);
	rwlock_initialize(&rwlock);
	
	for (i = 1; i <= 3; i++) {
		thread_t *thrd;
		
		atomic_set(&items_read, 0);
		atomic_set(&items_written, 0);
		
		readers = i * READERS;
		writers = (4 - i) * WRITERS;
		
		TPRINTF("Creating %ld readers and %ld writers...", readers, writers);
		
		for (j = 0; j < (READERS + WRITERS) / 2; j++) {
			for (k = 0; k < i; k++) {
				thrd = thread_create(reader, NULL, TASK, 0, "reader", false);
				if (thrd)
					thread_ready(thrd);
				else
					TPRINTF("Could not create reader %d\n", k);
			}
			for (k = 0; k < (4 - i); k++) {
				thrd = thread_create(writer, NULL, TASK, 0, "writer", false);
				if (thrd)
					thread_ready(thrd);
				else
					TPRINTF("Could not create writer %d\n", k);
			}
		}
		
		TPRINTF("ok\n");
		
		thread_sleep(1);
		waitq_wakeup(&can_start, WAKEUP_ALL);
		
		while ((items_read.count != readers) || (items_written.count != writers)) {
			TPRINTF("%d readers remaining, %d writers remaining, readers_in=%d\n", readers - items_read.count, writers - items_written.count, rwlock.readers_in);
			thread_usleep(100000);
		}
	}
	
	return NULL;
}
