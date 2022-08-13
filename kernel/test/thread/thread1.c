/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <atomic.h>
#include <stdbool.h>
#include <proc/thread.h>

#include <arch.h>

#define THREADS  5

static atomic_bool finish;
static atomic_size_t threads_finished;

static void threadtest(void *data)
{
	thread_detach(THREAD);

	while (atomic_load(&finish)) {
		TPRINTF("%" PRIu64 " ", THREAD->tid);
		thread_usleep(100000);
	}
	atomic_inc(&threads_finished);
}

const char *test_thread1(void)
{
	unsigned int i;
	size_t total = 0;

	atomic_store(&finish, true);
	atomic_store(&threads_finished, 0);

	for (i = 0; i < THREADS; i++) {
		thread_t *t;
		if (!(t = thread_create(threadtest, NULL, TASK,
		    THREAD_FLAG_NONE, "threadtest"))) {
			TPRINTF("Could not create thread %d\n", i);
			break;
		}
		thread_ready(t);
		total++;
	}

	TPRINTF("Running threads for 10 seconds...\n");
	thread_sleep(10);

	atomic_store(&finish, false);
	while (atomic_load(&threads_finished) < total) {
		TPRINTF("Threads left: %zu\n", total - atomic_load(&threads_finished));
		thread_sleep(1);
	}

	return NULL;
}
