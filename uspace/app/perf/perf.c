/*
 * Copyright (c) 2018 Jiri Svoboda
 * Copyright (c) 2018 Vojtech Horky
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

/** @addtogroup perf
 * @{
 */
/**
 * @file
 */

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <str.h>
#include <time.h>
#include <errno.h>
#include <perf.h>
#include <types/casting.h>
#include "perf.h"
#include "benchlist.h"

#define MIN_DURATION_SECS 10
#define NUM_SAMPLES 10
#define MAX_ERROR_STR_LENGTH 1024

static void short_report(stopwatch_t *stopwatch, int run_index,
    benchmark_t *bench, uint64_t workload_size)
{
	usec_t duration_usec = NSEC2USEC(stopwatch_get_nanos(stopwatch));

	printf("Completed %" PRIu64 " operations in %llu us",
	    workload_size, duration_usec);
	if (duration_usec > 0) {
		double nanos = stopwatch_get_nanos(stopwatch);
		double thruput = (double) workload_size / (nanos / 1000000000.0l);
		printf(", %.0f cycles/s.\n", thruput);
	} else {
		printf(".\n");
	}
}

static void summary_stats(stopwatch_t *stopwatch, size_t stopwatch_count,
    benchmark_t *bench, uint64_t workload_size)
{
	double sum = 0.0;
	double sum_square = 0.0;

	for (size_t i = 0; i < stopwatch_count; i++) {
		double nanos = stopwatch_get_nanos(&stopwatch[i]);
		double thruput = (double) workload_size / (nanos / 1000000000.0l);
		sum += thruput;
		sum_square += thruput * thruput;
	}

	double avg = sum / stopwatch_count;

	double sd_numer = sum_square + stopwatch_count * avg * avg - 2 * sum * avg;
	double sd_square = sd_numer / ((double) stopwatch_count - 1);

	printf("Average: %.0f cycles/s Std.dev^2: %.0f cycles/s Samples: %zu\n",
	    avg, sd_square, stopwatch_count);
}

static bool run_benchmark(benchmark_t *bench)
{
	printf("Warm up and determine workload size...\n");

	char *error_msg = malloc(MAX_ERROR_STR_LENGTH + 1);
	if (error_msg == NULL) {
		printf("Out of memory!\n");
		return false;
	}
	str_cpy(error_msg, MAX_ERROR_STR_LENGTH, "");

	bool ret = true;

	if (bench->setup != NULL) {
		ret = bench->setup(error_msg, MAX_ERROR_STR_LENGTH);
		if (!ret) {
			goto leave_error;
		}
	}

	/*
	 * Find workload size that is big enough to last few seconds.
	 * We also check that uint64_t is big enough.
	 */
	uint64_t workload_size = 0;
	for (size_t bits = 0; bits <= 64; bits++) {
		if (bits == 64) {
			str_cpy(error_msg, MAX_ERROR_STR_LENGTH, "Workload too small even for 1 << 63");
			goto leave_error;
		}
		workload_size = ((uint64_t) 1) << bits;

		stopwatch_t stopwatch = STOPWATCH_INITIALIZE_STATIC;

		bool ok = bench->entry(&stopwatch, workload_size,
		    error_msg, MAX_ERROR_STR_LENGTH);
		if (!ok) {
			goto leave_error;
		}
		short_report(&stopwatch, -1, bench, workload_size);

		nsec_t duration = stopwatch_get_nanos(&stopwatch);
		if (duration > SEC2NSEC(MIN_DURATION_SECS)) {
			break;
		}
	}

	printf("Workload size set to %" PRIu64 ", measuring %d samples.\n", workload_size, NUM_SAMPLES);

	stopwatch_t *stopwatch = calloc(NUM_SAMPLES, sizeof(stopwatch_t));
	if (stopwatch == NULL) {
		snprintf(error_msg, MAX_ERROR_STR_LENGTH, "failed allocating memory");
		goto leave_error;
	}
	for (int i = 0; i < NUM_SAMPLES; i++) {
		stopwatch_init(&stopwatch[i]);

		bool ok = bench->entry(&stopwatch[i], workload_size,
		    error_msg, MAX_ERROR_STR_LENGTH);
		if (!ok) {
			free(stopwatch);
			goto leave_error;
		}
		short_report(&stopwatch[i], i, bench, workload_size);
	}

	summary_stats(stopwatch, NUM_SAMPLES, bench, workload_size);
	printf("\nBenchmark completed\n");

	free(stopwatch);

	goto leave;

leave_error:
	printf("Error: %s\n", error_msg);
	ret = false;

leave:
	if (bench->teardown != NULL) {
		bool ok = bench->teardown(error_msg, MAX_ERROR_STR_LENGTH);
		if (!ok) {
			printf("Error: %s\n", error_msg);
			ret = false;
		}
	}

	free(error_msg);

	return ret;
}

static int run_benchmarks(void)
{
	unsigned int count_ok = 0;
	unsigned int count_fail = 0;

	char *failed_names = NULL;

	printf("\n*** Running all benchmarks ***\n\n");

	for (size_t it = 0; it < benchmark_count; it++) {
		printf("%s (%s)\n", benchmarks[it]->name, benchmarks[it]->desc);
		if (run_benchmark(benchmarks[it])) {
			count_ok++;
			continue;
		}

		if (!failed_names) {
			failed_names = str_dup(benchmarks[it]->name);
		} else {
			char *f = NULL;
			asprintf(&f, "%s, %s", failed_names, benchmarks[it]->name);
			if (!f) {
				printf("Out of memory.\n");
				abort();
			}
			free(failed_names);
			failed_names = f;
		}
		count_fail++;
	}

	printf("\nCompleted, %u benchmarks run, %u succeeded.\n",
	    count_ok + count_fail, count_ok);
	if (failed_names)
		printf("Failed benchmarks: %s\n", failed_names);

	return count_fail;
}

static void list_benchmarks(void)
{
	size_t len = 0;
	for (size_t i = 0; i < benchmark_count; i++) {
		size_t len_now = str_length(benchmarks[i]->name);
		if (len_now > len)
			len = len_now;
	}

	assert(can_cast_size_t_to_int(len) && "benchmark name length overflow");

	for (size_t i = 0; i < benchmark_count; i++)
		printf("%-*s %s\n", (int) len, benchmarks[i]->name, benchmarks[i]->desc);

	printf("%-*s Run all benchmarks\n", (int) len, "*");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage:\n\n");
		printf("%s <benchmark>\n\n", argv[0]);
		list_benchmarks();
		return 0;
	}

	if (str_cmp(argv[1], "*") == 0) {
		return run_benchmarks();
	}

	for (size_t i = 0; i < benchmark_count; i++) {
		if (str_cmp(argv[1], benchmarks[i]->name) == 0) {
			return (run_benchmark(benchmarks[i]) ? 0 : -1);
		}
	}

	printf("Unknown benchmark \"%s\"\n", argv[1]);
	return -2;
}

/** @}
 */
