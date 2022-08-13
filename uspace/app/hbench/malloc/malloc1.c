/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hbench
 * @{
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../hbench.h"

static bool runner(bench_env_t *env, bench_run_t *run, uint64_t size)
{
	bench_run_start(run);
	for (uint64_t i = 0; i < size; i++) {
		void *p = malloc(1);
		if (p == NULL) {
			return bench_run_fail(run,
			    "failed to allocate 1B in run %" PRIu64 " (out of %" PRIu64 ")",
			    i, size);
		}
		free(p);
	}
	bench_run_stop(run);

	return true;
}

benchmark_t benchmark_malloc1 = {
	.name = "malloc1",
	.desc = "User-space memory allocator benchmark, repeatedly allocate one block",
	.entry = &runner,
	.setup = NULL,
	.teardown = NULL
};

/** @}
 */
