/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hbench
 * @{
 */

#include <stdio.h>
#include <ipc_test.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include "../hbench.h"

static ipc_test_t *test = NULL;

static bool setup(bench_env_t *env, bench_run_t *run)
{
	errno_t rc = ipc_test_create(&test);
	if (rc != EOK) {
		return bench_run_fail(run,
		    "failed contacting IPC test server (have you run /srv/test/ipc-test?): %s (%d)",
		    str_error(rc), rc);
	}

	return true;
}

static bool teardown(bench_env_t *env, bench_run_t *run)
{
	ipc_test_destroy(test);
	return true;
}

static bool runner(bench_env_t *env, bench_run_t *run, uint64_t niter)
{
	bench_run_start(run);

	for (uint64_t count = 0; count < niter; count++) {
		errno_t rc = ipc_test_ping(test);

		if (rc != EOK) {
			return bench_run_fail(run, "failed sending ping message: %s (%d)",
			    str_error(rc), rc);
		}
	}

	bench_run_stop(run);

	return true;
}

benchmark_t benchmark_ping_pong = {
	.name = "ping_pong",
	.desc = "IPC ping-pong benchmark",
	.entry = &runner,
	.setup = &setup,
	.teardown = &teardown
};

/** @}
 */
