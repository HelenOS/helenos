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
