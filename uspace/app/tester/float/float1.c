/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdatomic.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <inttypes.h>
#include "../tester.h"

#define THREADS   150
#define ATTEMPTS  100

#define E_10E8     UINT32_C(271828182)
#define PRECISION  100000000

static FIBRIL_SEMAPHORE_INITIALIZE(threads_finished, 0);
static atomic_int threads_fault;

static errno_t e(void *data)
{
	for (unsigned int i = 0; i < ATTEMPTS; i++) {
		double le = -1;
		double e = 0;
		double f = 1;

		for (double d = 1; e != le; d *= f, f += 1) {
			le = e;
			e = e + 1 / d;
		}

		if ((uint32_t) (e * PRECISION) != E_10E8) {
			atomic_fetch_add(&threads_fault, 1);
			break;
		}
	}

	fibril_semaphore_up(&threads_finished);
	return EOK;
}

const char *test_float1(void)
{
	int total = 0;

	atomic_store(&threads_fault, 0);
	fibril_test_spawn_runners(THREADS);

	TPRINTF("Creating threads");
	for (unsigned int i = 0; i < THREADS; i++) {
		fid_t f = fibril_create(e, NULL);
		if (!f) {
			TPRINTF("\nCould not create thread %u\n", i);
			break;
		}
		fibril_detach(f);
		fibril_add_ready(f);

		TPRINTF(".");
		total++;
	}

	TPRINTF("\n");

	for (int i = 0; i < total; i++) {
		TPRINTF("Threads left: %d\n", total - i);
		fibril_semaphore_down(&threads_finished);
	}

	if (atomic_load(&threads_fault) == 0)
		return NULL;

	return "Test failed";
}
