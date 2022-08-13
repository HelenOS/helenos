/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hbench
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include "../hbench.h"

static bool runner(bench_env_t *env, bench_run_t *run, uint64_t niter)
{
	bench_run_start(run);

	void **p = malloc(niter * sizeof(void *));
	if (p == NULL) {
		return bench_run_fail(run, "failed to allocate backend array (%" PRIu64 "B)",
		    niter * sizeof(void *));
	}

	for (uint64_t count = 0; count < niter; count++) {
		p[count] = malloc(1);
		if (p[count] == NULL) {
			for (uint64_t j = 0; j < count; j++) {
				free(p[j]);
			}
			free(p);
			return bench_run_fail(run,
			    "failed to allocate 1B in run %" PRIu64 " (out of %" PRIu64 ")",
			    count, niter);
		}
	}

	for (uint64_t count = 0; count < niter; count++)
		free(p[count]);

	free(p);

	bench_run_stop(run);

	return true;
}

benchmark_t benchmark_malloc2 = {
	.name = "malloc2",
	.desc = "User-space memory allocator benchmark, allocate many small blocks",
	.entry = &runner,
	.setup = NULL,
	.teardown = NULL
};

/** @}
 */
