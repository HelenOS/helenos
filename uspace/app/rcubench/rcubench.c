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
#include <str.h>

#include <rcu.h>


/* Results are printed to this file in addition to stdout. */
static FILE *results_fd = NULL;

typedef struct bench {
	const char *name;
	void (*func)(struct bench *);
	size_t iters;
	size_t nthreads;
	futex_t done_threads;

	futex_t bench_fut;
} bench_t;




static void  kernel_futex_bench(bench_t *bench)
{
	const size_t iters = bench->iters;
	int val = 0;

	for (size_t i = 0; i < iters; ++i) {
		__SYSCALL1(SYS_FUTEX_WAKEUP, (sysarg_t) &val);
		__SYSCALL1(SYS_FUTEX_SLEEP, (sysarg_t) &val);
	}
}

static void libc_futex_lock_bench(bench_t *bench)
{
	const size_t iters = bench->iters;
	futex_t loc_fut = FUTEX_INITIALIZER;

	for (size_t i = 0; i < iters; ++i) {
		futex_lock(&loc_fut);
		/* no-op */
		compiler_barrier();
		futex_unlock(&loc_fut);
	}
}

static void libc_futex_sema_bench(bench_t *bench)
{
	const size_t iters = bench->iters;
	futex_t loc_fut = FUTEX_INITIALIZER;

	for (size_t i = 0; i < iters; ++i) {
		futex_down(&loc_fut);
		/* no-op */
		compiler_barrier();
		futex_up(&loc_fut);
	}
}

static void thread_func(void *arg)
{
	bench_t *bench = (bench_t *)arg;

	bench->func(bench);

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
		errno_t ret = thread_create(thread_func, bench, "rcubench-t", &tid);
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

static const char *results_txt = "/tmp/urcu-bench-results.txt";

static bool open_results(void)
{
	results_fd = fopen(results_txt, "a");
	return NULL != results_fd;
}

static void close_results(void)
{
	if (results_fd) {
		fclose(results_fd);
	}
}

static void print_res(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(results_fd, fmt, args);
	va_end(args);

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

static void print_usage(void)
{
	printf("rcubench [test-name] [k-iterations] [n-threads]\n");
	printf("Available tests: \n");
	printf("  sys-futex.. threads make wakeup/sleepdown futex syscalls in a loop\n");
	printf("              but for separate variables/futex kernel objects.\n");
	printf("  lock     .. threads lock/unlock separate futexes.\n");
	printf("  sema     .. threads down/up separate futexes.\n");
	printf("eg:\n");
	printf("  rcubench sys-futex  100000 3\n");
	printf("  rcubench lock 100000 2 ..runs futex_lock/unlock in a loop\n");
	printf("  rcubench sema 100000 2 ..runs futex_down/up in a loop\n");
	printf("Results are stored in %s\n", results_txt);
}

static bool parse_cmd_line(int argc, char **argv, bench_t *bench,
    const char **err)
{
	if (argc < 4) {
		*err = "Not enough parameters";
		return false;
	}

	futex_initialize(&bench->bench_fut, 1);

	if (0 == str_cmp(argv[1], "sys-futex")) {
		bench->func = kernel_futex_bench;
	} else if (0 == str_cmp(argv[1], "lock")) {
		bench->func = libc_futex_lock_bench;
	} else if (0 == str_cmp(argv[1], "sema")) {
		bench->func = libc_futex_sema_bench;
	} else {
		*err = "Unknown test name";
		return false;
	}

	bench->name = argv[1];

	/* Determine iteration count. */
	uint32_t iter_cnt = 0;
	errno_t ret = str_uint32_t(argv[2], NULL, 0, true, &iter_cnt);

	if (ret == EOK && 1 <= iter_cnt) {
		bench->iters = iter_cnt;
	} else {
		*err = "Err: Invalid number of iterations";
		return false;
	}

	/* Determine thread count. */
	uint32_t thread_cnt = 0;
	ret = str_uint32_t(argv[3], NULL, 0, true, &thread_cnt);

	if (ret == EOK && 1 <= thread_cnt && thread_cnt <= 64) {
		bench->nthreads = thread_cnt;
	} else {
		*err = "Err: Invalid number of threads";
		return false;
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

	open_results();

	print_res("Running '%s' futex bench in '%zu' threads with '%zu' iterations.\n",
	    bench.name, bench.nthreads, bench.iters);

	struct timeval start, end;
	getuptime(&start);

	run_threads_and_wait(&bench);

	getuptime(&end);
	int64_t duration = tv_sub_diff(&end, &start);

	uint64_t secs = (uint64_t)duration / 1000 / 1000;
	uint64_t total_iters = (uint64_t)bench.iters * bench.nthreads;
	uint64_t iters_per_sec = 0;

	if (0 < duration) {
		iters_per_sec = total_iters * 1000 * 1000 / duration;
	}

	print_res("Completed %" PRIu64 " iterations in %" PRId64  " usecs (%" PRIu64
	    " secs); %" PRIu64 " iters/sec\n",
	    total_iters, duration, secs, iters_per_sec);

	close_results();

	return 0;
}


/**
 * @}
 */
