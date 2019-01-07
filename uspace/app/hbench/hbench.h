/*
 * Copyright (c) 2018 Jiri Svoboda
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

#include <errno.h>
#include <stdbool.h>
#include <perf.h>

/*
 * So far, a simple wrapper around system stopwatch.
 * Eventually, we could collection of hardware counters etc. without
 * modifying signatures of any existing benchmark.
 */
typedef struct {
	stopwatch_t stopwatch;
} benchmeter_t;

static inline void benchmeter_init(benchmeter_t *meter)
{
	stopwatch_init(&meter->stopwatch);
}

static inline void benchmeter_start(benchmeter_t *meter)
{
	stopwatch_start(&meter->stopwatch);
}

static inline void benchmeter_stop(benchmeter_t *meter)
{
	stopwatch_stop(&meter->stopwatch);
}

typedef bool (*benchmark_entry_t)(benchmeter_t *, uint64_t,
    char *, size_t);
typedef bool (*benchmark_helper_t)(char *, size_t);

typedef struct {
	const char *name;
	const char *desc;
	benchmark_entry_t entry;
	benchmark_helper_t setup;
	benchmark_helper_t teardown;
} benchmark_t;

extern benchmark_t *benchmarks[];
extern size_t benchmark_count;

extern errno_t csv_report_open(const char *);
extern void csv_report_add_entry(benchmeter_t *, int, benchmark_t *, uint64_t);
extern void csv_report_close(void);

extern errno_t bench_param_init(void);
extern errno_t bench_param_set(const char *, const char *);
extern const char *bench_param_get(const char *, const char *);
extern void bench_param_cleanup(void);

/* Put your benchmark descriptors here (and also to benchlist.c). */
extern benchmark_t benchmark_dir_read;
extern benchmark_t benchmark_file_read;
extern benchmark_t benchmark_malloc1;
extern benchmark_t benchmark_malloc2;
extern benchmark_t benchmark_ns_ping;
extern benchmark_t benchmark_ping_pong;

#endif

/** @}
 */
