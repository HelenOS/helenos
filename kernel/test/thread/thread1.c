/*
 * Copyright (c) 2005 Jakub Vana
 * Copyright (c) 2005 Jakub Jermar
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
#include <atomic.h>
#include <stdbool.h>
#include <proc/thread.h>

#include <arch.h>

#define THREADS  5

static atomic_bool finish;

static void threadtest(void *data)
{
	while (atomic_load(&finish)) {
		TPRINTF("%" PRIu64 " ", THREAD->tid);
		thread_usleep(100000);
	}
}

const char *test_thread1(void)
{
	atomic_store(&finish, true);

	thread_t *threads[THREADS] = { };

	for (int i = 0; i < THREADS; i++) {
		threads[i] = thread_create(threadtest, NULL,
		    TASK, THREAD_FLAG_NONE, "threadtest");

		if (threads[i]) {
			thread_start(threads[i]);
		} else {
			TPRINTF("Could not create thread %d\n", i);
			break;
		}
	}

	TPRINTF("Running threads for 10 seconds...\n");
	thread_sleep(10);

	atomic_store(&finish, false);

	for (int i = 0; i < THREADS; i++) {
		if (threads[i] != NULL)
			thread_join(threads[i]);

		TPRINTF("Threads left: %d\n", THREADS - i - 1);
	}

	return NULL;
}
