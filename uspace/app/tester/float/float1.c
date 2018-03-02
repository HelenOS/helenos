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
#include <atomic.h>
#include <thread.h>
#include <inttypes.h>
#include "../tester.h"

#define THREADS   150
#define ATTEMPTS  100

#define E_10E8     UINT32_C(271828182)
#define PRECISION  100000000

static atomic_t threads_finished;
static atomic_t threads_fault;

static void e(void *data)
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
			atomic_inc(&threads_fault);
			break;
		}
	}

	atomic_inc(&threads_finished);
}

const char *test_float1(void)
{
	atomic_count_t total = 0;

	atomic_set(&threads_finished, 0);
	atomic_set(&threads_fault, 0);

	TPRINTF("Creating threads");
	for (unsigned int i = 0; i < THREADS; i++) {
		if (thread_create(e, NULL, "e", NULL) != EOK) {
			TPRINTF("\nCould not create thread %u\n", i);
			break;
		}

		TPRINTF(".");
		total++;
	}

	TPRINTF("\n");

	while (atomic_get(&threads_finished) < total) {
		TPRINTF("Threads left: %" PRIua "\n",
		    total - atomic_get(&threads_finished));
		thread_sleep(1);
	}

	if (atomic_get(&threads_fault) == 0)
		return NULL;

	return "Test failed";
}
