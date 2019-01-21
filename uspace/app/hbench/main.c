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

/** @addtogroup hbench
 * @{
 */
/**
 * @file
 */

#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <str.h>
#include <time.h>
#include <errno.h>
#include <str_error.h>
#include <perf.h>
#include <types/casting.h>
#include "hbench.h"

#define MAX_ERROR_STR_LENGTH 1024

static void short_report(bench_run_t *info, int run_index,
    benchmark_t *bench, uint64_t workload_size)
{
	csv_report_add_entry(info, run_index, bench, workload_size);

	usec_t duration_usec = NSEC2USEC(stopwatch_get_nanos(&info->stopwatch));

	printf("Completed %" PRIu64 " operations in %llu us",
	    workload_size, duration_usec);
	if (duration_usec > 0) {
		double nanos = stopwatch_get_nanos(&info->stopwatch);
		double thruput = (double) workload_size / (nanos / 1000000000.0l);
		printf(", %.0f ops/s.\n", thruput);
	} else {
		printf(".\n");
	}
}

/** Estimate square root value.
 *
 * @param value The value to compute square root of.
 * @param precision Required precision (e.g. 0.00001).
 *
 * @details
 *
 * This is a temporary solution until we have proper sqrt() implementation
 * in libmath.
 *
 * The algorithm uses Babylonian method [1].
 *
 * [1] https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Babylonian_method
 */
static double estimate_square_root(double value, double precision)
{
	double estimate = 1.;
	double prev_estimate = estimate + 10 * precision;

	while (fabs(estimate - prev_estimate) > precision) {
		prev_estimate = estimate;
		estimate = (prev_estimate + value / prev_estimate) / 2.;
	}

	return estimate;
}

/** Compute available statistics from given stopwatches.
 *
 * We compute normal mean for average duration of the workload and geometric
 * mean for average thruput. Note that geometric mean is necessary to compute
 * average throughput correctly - consider the following example:
 *  - we run always 60 operations,
 *  - first run executes in 30 s (i.e. 2 ops/s)
 *  - and second one in 10 s (6 ops/s).
 * Then, naively, average throughput would be (2+6)/2 = 4 [ops/s]. However, we
 * actually executed 60 + 60 ops in 30 + 10 seconds. So the actual average
 * throughput is 3 ops/s (which is exactly what geometric mean means).
 *
 */
static void compute_stats(bench_run_t *runs, size_t run_count,
    uint64_t workload_size, double precision, double *out_duration_avg,
    double *out_duration_sigma, double *out_thruput_avg)
{
	double inv_thruput_sum = 0.0;
	double nanos_sum = 0.0;
	double nanos_sum2 = 0.0;

	for (size_t i = 0; i < run_count; i++) {
		double nanos = stopwatch_get_nanos(&runs[i].stopwatch);
		double thruput = (double) workload_size / nanos;

		inv_thruput_sum += 1.0 / thruput;
		nanos_sum += nanos;
		nanos_sum2 += nanos * nanos;
	}
	*out_duration_avg = nanos_sum / run_count;
	double sigma2 = (nanos_sum2 - nanos_sum * (*out_duration_avg)) /
	    ((double) run_count - 1);
	// FIXME: implement sqrt properly
	if (run_count > 1) {
		*out_duration_sigma = estimate_square_root(sigma2, precision);
	} else {
		*out_duration_sigma = NAN;
	}
	*out_thruput_avg = 1.0 / (inv_thruput_sum / run_count);
}

static void summary_stats(bench_run_t *runs, size_t run_count,
    benchmark_t *bench, uint64_t workload_size)
{
	double duration_avg, duration_sigma, thruput_avg;
	compute_stats(runs, run_count, workload_size, 0.001,
	    &duration_avg, &duration_sigma, &thruput_avg);

	printf("Average: %" PRIu64 " ops in %.0f us (sd %.0f us); "
	    "%.0f ops/s; Samples: %zu\n",
	    workload_size, duration_avg / 1000.0, duration_sigma / 1000.0,
	    thruput_avg * 1000000000.0, run_count);
}

static bool run_benchmark(bench_env_t *env, benchmark_t *bench)
{
	printf("Warm up and determine workload size...\n");

	/*
	 * We share this buffer across all runs as we know that it is
	 * used only on failure (and we abort after first error).
	 */
	char *error_msg = malloc(MAX_ERROR_STR_LENGTH + 1);
	if (error_msg == NULL) {
		printf("Out of memory!\n");
		return false;
	}
	str_cpy(error_msg, MAX_ERROR_STR_LENGTH, "");

	bench_run_t helper_run;
	bench_run_init(&helper_run, error_msg, MAX_ERROR_STR_LENGTH);

	bool ret = true;

	if (bench->setup != NULL) {
		ret = bench->setup(env, &helper_run);
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

		bench_run_t run;
		bench_run_init(&run, error_msg, MAX_ERROR_STR_LENGTH);

		bool ok = bench->entry(env, &run, workload_size);
		if (!ok) {
			goto leave_error;
		}
		short_report(&run, -1, bench, workload_size);

		nsec_t duration = stopwatch_get_nanos(&run.stopwatch);
		if (duration > env->minimal_run_duration_nanos) {
			break;
		}
	}

	printf("Workload size set to %" PRIu64 ", measuring %zu samples.\n",
	    workload_size, env->run_count);

	bench_run_t *runs = calloc(env->run_count, sizeof(bench_run_t));
	if (runs == NULL) {
		snprintf(error_msg, MAX_ERROR_STR_LENGTH, "failed allocating memory");
		goto leave_error;
	}
	for (size_t i = 0; i < env->run_count; i++) {
		bench_run_init(&runs[i], error_msg, MAX_ERROR_STR_LENGTH);

		bool ok = bench->entry(env, &runs[i], workload_size);
		if (!ok) {
			free(runs);
			goto leave_error;
		}
		short_report(&runs[i], i, bench, workload_size);
	}

	summary_stats(runs, env->run_count, bench, workload_size);
	printf("\nBenchmark completed\n");

	free(runs);

	goto leave;

leave_error:
	printf("Error: %s\n", error_msg);
	ret = false;

leave:
	if (bench->teardown != NULL) {
		bool ok = bench->teardown(env, &helper_run);
		if (!ok) {
			printf("Error: %s\n", error_msg);
			ret = false;
		}
	}

	free(error_msg);

	return ret;
}

static int run_benchmarks(bench_env_t *env)
{
	unsigned int count_ok = 0;
	unsigned int count_fail = 0;

	char *failed_names = NULL;

	printf("\n*** Running all benchmarks ***\n\n");

	for (size_t it = 0; it < benchmark_count; it++) {
		printf("%s (%s)\n", benchmarks[it]->name, benchmarks[it]->desc);
		if (run_benchmark(env, benchmarks[it])) {
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
		printf("  %-*s %s\n", (int) len, benchmarks[i]->name, benchmarks[i]->desc);

	printf("  %-*s Run all benchmarks\n", (int) len, "*");
}

static void print_usage(const char *progname)
{
	printf("Usage: %s [options] <benchmark>\n", progname);
	printf("-h, --help                 "
	    "Print this help and exit\n");
	printf("-d, --duration MILLIS      "
	    "Set minimal run duration (milliseconds)\n");
	printf("-n, --count N              "
	    "Set number of measured runs\n");
	printf("-o, --output filename.csv  "
	    "Store machine-readable data in filename.csv\n");
	printf("-p, --param KEY=VALUE      "
	    "Additional parameters for the benchmark\n");
	printf("<benchmark> is one of the following:\n");
	list_benchmarks();
}

static void handle_param_arg(bench_env_t *env, char *arg)
{
	char *value = NULL;
	char *key = str_tok(arg, "=", &value);
	bench_env_param_set(env, key, value);
}

int main(int argc, char *argv[])
{
	bench_env_t bench_env;
	errno_t rc = bench_env_init(&bench_env);
	if (rc != EOK) {
		fprintf(stderr, "Failed to initialize internal params structure: %s\n",
		    str_error(rc));
		return -5;
	}

	const char *short_options = "ho:p:n:d:";
	struct option long_options[] = {
		{ "duration", required_argument, NULL, 'd' },
		{ "help", optional_argument, NULL, 'h' },
		{ "count", required_argument, NULL, 'n' },
		{ "output", required_argument, NULL, 'o' },
		{ "param", required_argument, NULL, 'p' },
		{ 0, 0, NULL, 0 }
	};

	char *csv_output_filename = NULL;

	int opt = 0;
	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) > 0) {
		switch (opt) {
		case 'd':
			errno = EOK;
			bench_env.minimal_run_duration_nanos = MSEC2NSEC(atoll(optarg));
			if ((errno != EOK) || (bench_env.minimal_run_duration_nanos <= 0)) {
				fprintf(stderr, "Invalid -d argument.\n");
				return -3;
			}
			break;
		case 'h':
			print_usage(*argv);
			return 0;
		case 'n':
			errno = EOK;
			bench_env.run_count = (nsec_t) atoll(optarg);
			if ((errno != EOK) || (bench_env.run_count <= 0)) {
				fprintf(stderr, "Invalid -n argument.\n");
				return -3;
			}
			break;
		case 'o':
			csv_output_filename = optarg;
			break;
		case 'p':
			handle_param_arg(&bench_env, optarg);
			break;
		case -1:
		default:
			break;
		}
	}

	if (optind + 1 != argc) {
		print_usage(*argv);
		fprintf(stderr, "Error: specify one benchmark to run or * for all.\n");
		return -3;
	}

	const char *benchmark = argv[optind];

	if (csv_output_filename != NULL) {
		errno_t rc = csv_report_open(csv_output_filename);
		if (rc != EOK) {
			fprintf(stderr, "Failed to open CSV report '%s': %s\n",
			    csv_output_filename, str_error(rc));
			return -4;
		}
	}

	int exit_code = 0;

	if (str_cmp(benchmark, "*") == 0) {
		exit_code = run_benchmarks(&bench_env);
	} else {
		bool benchmark_exists = false;
		for (size_t i = 0; i < benchmark_count; i++) {
			if (str_cmp(benchmark, benchmarks[i]->name) == 0) {
				benchmark_exists = true;
				exit_code = run_benchmark(&bench_env, benchmarks[i]) ? 0 : -1;
				break;
			}
		}
		if (!benchmark_exists) {
			printf("Unknown benchmark \"%s\"\n", benchmark);
			exit_code = -2;
		}
	}

	csv_report_close();
	bench_env_cleanup(&bench_env);

	return exit_code;
}

/** @}
 */
