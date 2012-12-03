/*
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup test
 * @{
 */

/**
 * @file rcubench.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mem.h>
#include <errno.h>
#include <thread.h>
#include <assert.h>
#include <async.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <compiler/barrier.h>
#include <futex.h>

#include <rcu.h>

typedef struct bench {
	enum {
		T_KERN_FUTEX,
		T_LIBC_FUTEX
	} type;
	size_t iters;
	size_t nthreads;
	size_t array_size;
	size_t *array;
	futex_t done_threads;
	
	futex_t ke_bench_fut;
	fibril_mutex_t libc_bench_mtx;
} bench_t;


/* Combats compiler optimizations. */
static volatile size_t dummy = 0;

static size_t sum_array(size_t *array, size_t len)
{
	size_t sum = 0;
	
	for (size_t k = 0; k < len; ++k)
		sum += array[k];
	
	return sum;
}

static void  kernel_futex_bench(bench_t *bench)
{
	futex_t * const fut = &bench->ke_bench_fut;
	const size_t iters = bench->iters;
	size_t sum = 0;
	
	for (size_t i = 0; i < iters; ++i) {
		/* Do some work with the futex locked to encourage contention. */
		futex_down(fut);
		sum += sum_array(bench->array, bench->array_size);
		futex_up(fut);
		
		/* 
		 * Do half as much work to give other threads a chance to acquire 
		 * the futex.
		 */
		sum += sum_array(bench->array, bench->array_size / 2);
	}
	
	/* 
	 * Writing to a global volatile variable separated with a cc-barrier
	 * should discourage the compiler from optimizing away sum_array()s.
	 */
	compiler_barrier();
	dummy = sum;
}

static void libc_futex_bench(bench_t *bench)
{
	fibril_mutex_t * const mtx = &bench->libc_bench_mtx;
	const size_t iters = bench->iters;
	
	for (size_t i = 0; i < iters; ++i) {
		fibril_mutex_lock(mtx);
		/* no-op */
		compiler_barrier();
		fibril_mutex_unlock(mtx);
	}
}


static void thread_func(void *arg)
{
	bench_t *bench = (bench_t*)arg;
	assert(bench->type == T_KERN_FUTEX || bench->type == T_LIBC_FUTEX);
	
	if (bench->type == T_KERN_FUTEX) 
		kernel_futex_bench(bench);
	else
		libc_futex_bench(bench);
	
	/* Signal another thread completed. */
	futex_up(&bench->done_threads);
}

static void run_threads_and_wait(bench_t *bench)
{
	assert(1 <= bench->nthreads);
	
	if (2 <= bench->nthreads) {
		printf("Creating %zu additional threads...\n", bench->nthreads - 1);
	}
	
	/* Create and run the first nthreads - 1 threads.*/
	for (size_t k = 1; k < bench->nthreads; ++k) {
		thread_id_t tid;
		/* Also sets up a fibril for the thread. */
		int ret = thread_create(thread_func, bench, "rcubench-t", &tid);
		if (ret != EOK) {
			printf("Error: Failed to create benchmark thread.\n");
			abort();
		}
		thread_detach(tid);
	}
	
	/* 
	 * Run the last thread in place so that we create multiple threads
	 * only when needed. Otherwise libc would immediately upgrade
	 * single-threaded futexes to proper multithreaded futexes
	 */
	thread_func(bench);
	
	printf("Waiting for remaining threads to complete.\n");
	
	/* Wait for threads to complete. */
	for (size_t k = 0; k < bench->nthreads; ++k) {
		futex_down(&bench->done_threads);
	}
}

static void print_usage(void)
{
	printf("rcubench [test-name] [k-iterations] [n-threads] {work-size}\n");
	printf("eg:\n");
	printf("  rcubench ke   100000 3 4\n");
	printf("  rcubench libc 100000 2\n");
	printf("  rcubench def-ke  \n");
	printf("  rcubench def-libc\n");
}

static bool parse_cmd_line(int argc, char **argv, bench_t *bench, 
	const char **err)
{
	if (argc < 2) {
		*err = "Benchmark name not specified";
		return false;
	}

	futex_initialize(&bench->ke_bench_fut, 1);
	fibril_mutex_initialize(&bench->libc_bench_mtx);
	
	if (0 == str_cmp(argv[1], "def-ke")) {
		bench->type = T_KERN_FUTEX;
		bench->nthreads = 4;
		bench->iters = 1000 * 1000;
		bench->array_size = 10;
		bench->array = malloc(bench->array_size * sizeof(size_t));
		return NULL != bench->array;
	} else if (0 == str_cmp(argv[1], "def-libc")) {
		bench->type = T_LIBC_FUTEX;
		bench->nthreads = 4;
		bench->iters = 1000 * 1000;
		bench->array_size = 0;
		bench->array = NULL;
		return true;
	} else if (0 == str_cmp(argv[1], "ke")) {
		bench->type = T_KERN_FUTEX;
	} else if (0 == str_cmp(argv[1], "libc")) {
		bench->type = T_LIBC_FUTEX;
	} else {
		*err = "Unknown test name";
		return false;
	}
	
	if (argc < 4) {
		*err = "Not enough parameters";
		return false;
	}
	
	uint32_t iter_cnt = 0;
	int ret = str_uint32_t(argv[2], NULL, 0, true, &iter_cnt);

	if (ret == EOK && 1 <= iter_cnt) {
		bench->iters = iter_cnt;
	} else {
		*err = "Err: Invalid number of iterations";
		return false;
	} 
	
	uint32_t thread_cnt = 0;
	ret = str_uint32_t(argv[3], NULL, 0, true, &thread_cnt);

	if (ret == EOK && 1 <= thread_cnt && thread_cnt <= 64) {
		bench->nthreads = thread_cnt;
	} else {
		*err = "Err: Invalid number of threads";
		return false;
	} 
	
	if (argc > 4) {
		uint32_t work_size = 0;
		ret = str_uint32_t(argv[4], NULL, 0, true, &work_size);

		if (ret == EOK && work_size <= 10000) {
			bench->array_size = work_size;
		} else {
			*err = "Err: Work size too large";
			return false;
		} 
	} else {
		bench->array_size = 0;
	}
	
	if (0 < bench->array_size) {
		bench->array = malloc(bench->array_size * sizeof(size_t));
		if (!bench->array) {
			*err = "Err: Failed to allocate work array";
			return false;
		}
	} else {
		bench->array = NULL;
	}
	
	return true;
}

int main(int argc, char **argv)
{
	const char *err = "(error)";
	bench_t bench;
	
	futex_initialize(&bench.done_threads, 0);
	
	if (!parse_cmd_line(argc, argv, &bench, &err)) {
		printf("%s\n", err);
		print_usage();
		return -1;
	}
	
	printf("Running '%s' futex bench in '%zu' threads with '%zu' iterations.\n",
		bench.type == T_KERN_FUTEX ? "kernel" : "libc", 
		bench.nthreads, bench.iters);
	
	struct timeval start, end;
	getuptime(&start);
	
	run_threads_and_wait(&bench);
	
	getuptime(&end);
	int64_t duration = tv_sub(&end, &start);
	
	uint64_t total_iters = (uint64_t)bench.iters * bench.nthreads;
	uint64_t iters_per_sec = total_iters * 1000 * 1000 / duration;
	uint64_t secs = (uint64_t)duration / 1000 / 1000;
	
	printf("Completed %" PRIu64 " iterations in %" PRId64  " usecs (%" PRIu64 
		" secs); %" PRIu64 " iters/sec\n", 
		total_iters, duration, secs, iters_per_sec);	
	
	return 0;
}


/**
 * @}
 */
