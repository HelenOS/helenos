/*
 * SPDX-FileCopyrightText: 2006 Sergey Bondari
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <stdlib.h>
#include <arch/mm/page.h>
#include <typedefs.h>
#include <atomic.h>
#include <proc/thread.h>
#include <mem.h>
#include <arch.h>

#define MAX_FRAMES  256

#define THREAD_RUNS  1
#define THREADS      8

static atomic_size_t thread_cnt;
static atomic_size_t thread_fail;

static void falloc(void *arg)
{
	uint8_t val = THREAD->tid % THREADS;

	uintptr_t *frames = (uintptr_t *)
	    malloc(MAX_FRAMES * sizeof(uintptr_t));
	if (frames == NULL) {
		TPRINTF("Thread #%" PRIu64 " (cpu%u): "
		    "Unable to allocate frames\n", THREAD->tid, CPU->id);
		atomic_inc(&thread_fail);
		atomic_dec(&thread_cnt);
		return;
	}

	thread_detach(THREAD);

	for (unsigned int run = 0; run < THREAD_RUNS; run++) {
		for (size_t count = 1; count <= MAX_FRAMES; count++) {
			size_t bytes = FRAMES2SIZE(count);

			TPRINTF("Thread #%" PRIu64 " (cpu%u): "
			    "Allocating %zu frames blocks (%zu bytes) ... \n", THREAD->tid,
			    CPU->id, count, bytes);

			unsigned int allocated = 0;
			for (unsigned int i = 0; i < (MAX_FRAMES / count); i++) {
				frames[allocated] = frame_alloc(count, FRAME_ATOMIC, 0);
				if (frames[allocated]) {
					memsetb((void *) PA2KA(frames[allocated]), bytes, val);
					allocated++;
				} else
					break;
			}

			TPRINTF("Thread #%" PRIu64 " (cpu%u): "
			    "%u blocks allocated.\n", THREAD->tid, CPU->id,
			    allocated);
			TPRINTF("Thread #%" PRIu64 " (cpu%u): "
			    "Deallocating ... \n", THREAD->tid, CPU->id);

			for (unsigned int i = 0; i < allocated; i++) {
				for (size_t k = 0; k < bytes; k++) {
					if (((uint8_t *) PA2KA(frames[i]))[k] != val) {
						TPRINTF("Thread #%" PRIu64 " (cpu%u): "
						    "Unexpected data (%c) in block %zu offset %zu\n",
						    THREAD->tid, CPU->id, ((char *) PA2KA(frames[i]))[k],
						    frames[i], k);
						atomic_inc(&thread_fail);
						goto cleanup;
					}
				}
				frame_free(frames[i], count);
			}

			TPRINTF("Thread #%" PRIu64 " (cpu%u): "
			    "Finished run.\n", THREAD->tid, CPU->id);
		}
	}

cleanup:
	free(frames);

	TPRINTF("Thread #%" PRIu64 " (cpu%u): Exiting\n",
	    THREAD->tid, CPU->id);
	atomic_dec(&thread_cnt);
}

const char *test_falloc2(void)
{
	atomic_store(&thread_cnt, THREADS);
	atomic_store(&thread_fail, 0);

	for (unsigned int i = 0; i < THREADS; i++) {
		thread_t *thrd = thread_create(falloc, NULL, TASK,
		    THREAD_FLAG_NONE, "falloc2");
		if (!thrd) {
			TPRINTF("Could not create thread %u\n", i);
			break;
		}
		thread_ready(thrd);
	}

	while (atomic_load(&thread_cnt) > 0) {
		TPRINTF("Threads left: %zu\n",
		    atomic_load(&thread_cnt));
		thread_sleep(1);
	}

	if (atomic_load(&thread_fail) == 0)
		return NULL;

	return "Test failed";
}
