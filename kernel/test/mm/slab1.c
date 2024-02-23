/*
 * Copyright (c) 2006 Ondrej Palkovsky
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
#include <mm/slab.h>
#include <proc/thread.h>
#include <arch.h>
#include <memw.h>

#define VAL_COUNT  1024

static void *data[VAL_COUNT];

static void testit(int size, int count)
{
	slab_cache_t *cache;
	int i;

	TPRINTF("Creating cache, object size: %d.\n", size);

	cache = slab_cache_create("test_cache", size, 0, NULL, NULL,
	    SLAB_CACHE_NOMAGAZINE);

	TPRINTF("Allocating %d items...", count);

	for (i = 0; i < count; i++) {
		data[i] = slab_alloc(cache, FRAME_ATOMIC);
		if (data[i])
			memsetb(data[i], size, 0);
	}

	TPRINTF("done.\n");

	TPRINTF("Freeing %d items...", count);

	for (i = 0; i < count; i++)
		slab_free(cache, data[i]);

	TPRINTF("done.\n");

	TPRINTF("Allocating %d items...", count);

	for (i = 0; i < count; i++) {
		data[i] = slab_alloc(cache, FRAME_ATOMIC);
		if (data[i])
			memsetb(data[i], size, 0);
	}

	TPRINTF("done.\n");

	TPRINTF("Freeing %d items...", count / 2);

	for (i = count - 1; i >= count / 2; i--)
		slab_free(cache, data[i]);

	TPRINTF("done.\n");

	TPRINTF("Allocating %d items...", count / 2);

	for (i = count / 2; i < count; i++) {
		data[i] = slab_alloc(cache, FRAME_ATOMIC);
		if (data[i])
			memsetb(data[i], size, 0);
	}

	TPRINTF("done.\n");

	TPRINTF("Freeing %d items...", count);

	for (i = 0; i < count; i++)
		slab_free(cache, data[i]);

	TPRINTF("done.\n");

	slab_cache_destroy(cache);

	TPRINTF("Test complete.\n");
}

static void testsimple(void)
{
	testit(100, VAL_COUNT);
	testit(200, VAL_COUNT);
	testit(1024, VAL_COUNT);
	testit(2048, 512);
	testit(4000, 128);
	testit(8192, 128);
	testit(16384, 128);
	testit(16385, 128);
}

#define THREADS        6
#define THR_MEM_COUNT  1024
#define THR_MEM_SIZE   128

static void *thr_data[THREADS][THR_MEM_COUNT];
static slab_cache_t *thr_cache;

static void slabtest(void *data)
{
	int offs = (int) (sysarg_t) data;
	int i, j;

	TPRINTF("Starting thread #%" PRIu64 "...\n", THREAD->tid);

	for (j = 0; j < 10; j++) {
		for (i = 0; i < THR_MEM_COUNT; i++)
			thr_data[offs][i] = slab_alloc(thr_cache, FRAME_ATOMIC);
		for (i = 0; i < THR_MEM_COUNT / 2; i++)
			slab_free(thr_cache, thr_data[offs][i]);
		for (i = 0; i < THR_MEM_COUNT / 2; i++)
			thr_data[offs][i] = slab_alloc(thr_cache, FRAME_ATOMIC);
		for (i = 0; i < THR_MEM_COUNT; i++)
			slab_free(thr_cache, thr_data[offs][i]);
	}

	TPRINTF("Thread #%" PRIu64 " finished\n", THREAD->tid);
}

static void testthreads(void)
{
	thr_cache = slab_cache_create("thread_cache", THR_MEM_SIZE, 0, NULL, NULL,
	    SLAB_CACHE_NOMAGAZINE);

	thread_t *threads[THREADS] = { };

	for (int i = 0; i < THREADS; i++) {
		threads[i] = thread_create(slabtest, (void *) (sysarg_t) i,
		    TASK, THREAD_FLAG_NONE, "slabtest");
		if (threads[i]) {
			thread_start(threads[i]);
		} else {
			TPRINTF("Could not create thread %d\n", i);
		}
	}

	for (int i = 0; i < THREADS; i++) {
		if (threads[i] != NULL)
			thread_join(threads[i]);
	}

	slab_cache_destroy(thr_cache);

	TPRINTF("Test complete.\n");
}

const char *test_slab1(void)
{
	testsimple();
	testthreads();

	return NULL;
}
