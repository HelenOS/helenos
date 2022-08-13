/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <mm/slab.h>
#include <proc/thread.h>
#include <arch.h>
#include <mem.h>

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
static semaphore_t thr_sem;

static void slabtest(void *data)
{
	int offs = (int) (sysarg_t) data;
	int i, j;

	thread_detach(THREAD);

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

	semaphore_up(&thr_sem);
}

static void testthreads(void)
{
	thread_t *t;
	int i;

	thr_cache = slab_cache_create("thread_cache", THR_MEM_SIZE, 0, NULL, NULL,
	    SLAB_CACHE_NOMAGAZINE);

	semaphore_initialize(&thr_sem, 0);
	for (i = 0; i < THREADS; i++) {
		if (!(t = thread_create(slabtest, (void *) (sysarg_t) i, TASK, THREAD_FLAG_NONE, "slabtest"))) {
			TPRINTF("Could not create thread %d\n", i);
		} else
			thread_ready(t);
	}

	for (i = 0; i < THREADS; i++)
		semaphore_down(&thr_sem);

	slab_cache_destroy(thr_cache);

	TPRINTF("Test complete.\n");
}

const char *test_slab1(void)
{
	testsimple();
	testthreads();

	return NULL;
}
