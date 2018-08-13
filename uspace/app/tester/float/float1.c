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
