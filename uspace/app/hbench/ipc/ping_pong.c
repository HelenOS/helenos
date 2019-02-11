/*
 * Copyright (c) 2009 Jiri Svoboda
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
