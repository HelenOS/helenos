/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2019 Vojtech Horky
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
/** @file
 */

#ifndef HBENCH_H_
#define HBENCH_H_

#include <adt/hash_table.h>
#include <errno.h>
#include <stdbool.h>
#include <perf.h>

#define DEFAULT_RUN_COUNT 10
#define DEFAULT_MIN_RUN_DURATION_MSEC 1000

/** Single run information.
 *
 * Used to store both performance information (now, only wall-clock
 * time) as well as information about error.
 *
 * Use proper access functions when modifying data inside this structure.
 *
 * Eventually, we could collection of hardware counters etc. without
 * modifying signatures of any existing benchmark.
 */
typedef struct {
	stopwatch_t stopwatch;
	char *error_message;
	size_t error_message_buffer_size;
} bench_run_t;

/** Benchmark environment configuration.
 *
 * Benchmarking code (runners) should use access functions to read
 * data from this structure (now only bench_env_param_get).
 *
 * Harness can access it directly.
 */
typedef struct {
	hash_table_t parameters;
	size_t run_count;
	nsec_t minimal_run_duration_nanos;
} bench_env_t;

/** Actual benchmark runner.
 *
 * The first argument describes the environment, second is used to store
 * information about the run (performance data or error message) and
 * third describes workload size (number of iterations).
 */
typedef bool (*benchmark_entry_t)(bench_env_t *, bench_run_t *, uint64_t);

/** Setup and teardown callback type.
 *
 * Unlike in benchmark_entry_t, we do not need to pass in number of
 * iterations to execute (note that we use bench_run_t only to simplify
 * creation of error messages).
 */
typedef bool (*benchmark_helper_t)(bench_env_t *, bench_run_t *);

typedef struct {
	const char *name;
	const char *desc;
	benchmark_entry_t entry;
	benchmark_helper_t setup;
	benchmark_helper_t teardown;
} benchmark_t;

extern void bench_run_init(bench_run_t *, char *, size_t);
extern bool bench_run_fail(bench_run_t *, const char *, ...);

/*
 * We keep the following two functions inline to ensure that we start
 * measurement as close to the surrounding code as possible. Note that
 * this inlining is done at least on the level of individual wrappers
 * (this one and the one provided by stopwatch_t) to pass the control
 * as fast as possible to the actual timer used.
 */

static inline void bench_run_start(bench_run_t *run)
{
	stopwatch_start(&run->stopwatch);
}

static inline void bench_run_stop(bench_run_t *run)
{
	stopwatch_stop(&run->stopwatch);
}

extern errno_t csv_report_open(const char *);
extern void csv_report_add_entry(bench_run_t *, int, benchmark_t *, uint64_t);
extern void csv_report_close(void);

extern errno_t bench_env_init(bench_env_t *);
extern errno_t bench_env_param_set(bench_env_t *, const char *, const char *);
extern const char *bench_env_param_get(bench_env_t *, const char *, const char *);
extern void bench_env_cleanup(bench_env_t *);

extern benchmark_t *benchmarks[];
extern size_t benchmark_count;

/* Put your benchmark descriptors here (and also to benchlist.c). */
extern benchmark_t benchmark_dir_read;
extern benchmark_t benchmark_fibril_mutex;
extern benchmark_t benchmark_file_read;
extern benchmark_t benchmark_rand_read;
extern benchmark_t benchmark_seq_read;
extern benchmark_t benchmark_malloc1;
extern benchmark_t benchmark_malloc2;
extern benchmark_t benchmark_ns_ping;
extern benchmark_t benchmark_ping_pong;
extern benchmark_t benchmark_read1k;
extern benchmark_t benchmark_taskgetid;
extern benchmark_t benchmark_write1k;

#endif

/** @}
 */
