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

#define THREADS 5

#include <atomic.h>
#include <thread.h>
#include <stdio.h>
#include <unistd.h>
#include "../tester.h"

static atomic_t finish;
static atomic_t threads_finished;
static bool sh_quiet;

static void threadtest(void *data)
{
	thread_detach(thread_get_id());

	while (atomic_get(&finish)) {
		if (!sh_quiet)
			printf("%d ", thread_get_id());
		usleep(100000);
	}
	atomic_inc(&threads_finished);
}

char * test_thread1(bool quiet)
{
	unsigned int i, total = 0;
	sh_quiet = quiet;
	
	atomic_set(&finish, 1);
	atomic_set(&threads_finished, 0);

	for (i = 0; i < THREADS; i++) {  
		int t;
		if ((t = thread_create(threadtest, NULL, "threadtest")) < 0) {
			if (!quiet)
				printf("Could not create thread %d\n", i);
			break;
		}
		total++;
	}
	
	if (!quiet)
		printf("Running threads for 10 seconds...\n");
	sleep(10);
	
	atomic_set(&finish, 0);
	while (atomic_get(&threads_finished) < total) {
		if (!quiet)
			printf("Threads left: %d\n", total - atomic_get(&threads_finished));
		sleep(1);
	}
	
	return NULL;
}
