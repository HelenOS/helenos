/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 * SPDX-FileCopyrightText: 2019 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hbench
 * @{
 */

#include <dirent.h>
#include <str_error.h>
#include <stdio.h>
#include <stdlib.h>
#include "../hbench.h"

/** Execute directory listing benchmark.
 *
 * Note that while this benchmark tries to measure speed of direct
 * read, it rather measures speed of FS cache as it is highly probable
 * that the corresponding blocks would be cached after first run.
 */
static bool runner(bench_env_t *env, bench_run_t *run, uint64_t size)
{
	const char *path = bench_env_param_get(env, "dirname", "/");

	bench_run_start(run);
	for (uint64_t i = 0; i < size; i++) {
		DIR *dir = opendir(path);
		if (dir == NULL) {
			return bench_run_fail(run, "failed to open %s for reading: %s",
			    path, str_error(errno));
		}

		struct dirent *dp;
		while ((dp = readdir(dir))) {
			/* Do nothing */
		}

		closedir(dir);
	}
	bench_run_stop(run);

	return true;
}

benchmark_t benchmark_dir_read = {
	.name = "dir_read",
	.desc = "Read contents of a directory (use 'dirname' param to alter the default).",
	.entry = &runner,
	.setup = NULL,
	.teardown = NULL
};

/**
 * @}
 */
