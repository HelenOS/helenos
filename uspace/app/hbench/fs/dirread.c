/*
 * Copyright (c) 2011 Martin Sucha
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
