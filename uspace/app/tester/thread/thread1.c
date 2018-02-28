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

#define THREADS  20
#define DELAY    10

#include <atomic.h>
#include <errno.h>
#include <thread.h>
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include "../tester.h"

static atomic_t finish;
static atomic_t threads_finished;

static void threadtest(void *data)
{
	thread_detach(thread_get_id());

	while (atomic_get(&finish))
		thread_usleep(100000);

	atomic_inc(&threads_finished);
}

const char *test_thread1(void)
{
	unsigned int i;
	atomic_count_t total = 0;

	atomic_set(&finish, 1);
	atomic_set(&threads_finished, 0);

	TPRINTF("Creating threads");
	for (i = 0; i < THREADS; i++) {
		if (thread_create(threadtest, NULL, "threadtest", NULL) != EOK) {
			TPRINTF("\nCould not create thread %u\n", i);
			break;
		}
		TPRINTF(".");
		total++;
	}

	TPRINTF("\nRunning threads for %u seconds...", DELAY);
	thread_sleep(DELAY);
	TPRINTF("\n");

	atomic_set(&finish, 0);
	while (atomic_get(&threads_finished) < total) {
		TPRINTF("Threads left: %" PRIua "\n",
		    total - atomic_get(&threads_finished));
		thread_sleep(1);
	}

	return NULL;
}
