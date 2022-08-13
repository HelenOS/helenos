/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hbench
 * @{
 */

#include <stdio.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include "../hbench.h"

static bool runner(bench_env_t *env, bench_run_t *run, uint64_t niter)
{
	bench_run_start(run);

	for (uint64_t count = 0; count < niter; count++) {
		errno_t rc = ns_ping();

		if (rc != EOK) {
			return bench_run_fail(run, "failed sending ping message: %s (%d)",
			    str_error(rc), rc);
		}
	}

	bench_run_stop(run);

	return true;
}

benchmark_t benchmark_ns_ping = {
	.name = "ns_ping",
	.desc = "Name service IPC ping-pong benchmark",
	.entry = &runner,
	.setup = NULL,
	.teardown = NULL
};

/** @}
 */
