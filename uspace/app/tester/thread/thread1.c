/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define THREADS  20
#define DELAY    10

#include <stdatomic.h>
#include <errno.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include "../tester.h"

static atomic_bool finish;

static FIBRIL_SEMAPHORE_INITIALIZE(threads_finished, 0);

static errno_t threadtest(void *data)
{
	fibril_detach(fibril_get_id());

	while (!atomic_load(&finish))
		fibril_usleep(100000);

	fibril_semaphore_up(&threads_finished);
	return EOK;
}

const char *test_thread1(void)
{
	int total = 0;

	atomic_store(&finish, false);

	fibril_test_spawn_runners(THREADS);

	TPRINTF("Creating threads");
	for (int i = 0; i < THREADS; i++) {
		fid_t f = fibril_create(threadtest, NULL);
		if (!f) {
			TPRINTF("\nCould not create thread %u\n", i);
			break;
		}
		fibril_add_ready(f);
		TPRINTF(".");
		total++;
	}

	TPRINTF("\nRunning threads for %u seconds...", DELAY);
	fibril_sleep(DELAY);
	TPRINTF("\n");

	atomic_store(&finish, true);
	for (int i = 0; i < total; i++) {
		TPRINTF("Threads left: %d\n", total - i);
		fibril_semaphore_down(&threads_finished);
	}

	return NULL;
}
