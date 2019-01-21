/*
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

#include <fibril_synch.h>
#include <stdatomic.h>
#include "../hbench.h"

/*
 * Simple benchmark for fibril mutexes. There are two fibrils that compete
 * over the same mutex as that is the simplest scenario.
 */

typedef struct {
	fibril_mutex_t mutex;
	uint64_t counter;
	atomic_bool done;
} shared_t;

static errno_t competitor(void *arg)
{
	shared_t *shared = arg;
	fibril_detach(fibril_get_id());

	while (true) {
		fibril_mutex_lock(&shared->mutex);
		uint64_t local = shared->counter;
		fibril_mutex_unlock(&shared->mutex);
		if (local == 0) {
			break;
		}
	}

	atomic_store(&shared->done, true);

	return EOK;
}

static bool runner(bench_env_t *env, bench_run_t *run, uint64_t size)
{
	shared_t shared;
	fibril_mutex_initialize(&shared.mutex);
	shared.counter = size;
	atomic_store(&shared.done, false);

	fid_t other = fibril_create(competitor, &shared);
	fibril_add_ready(other);

	bench_run_start(run);
	for (uint64_t i = 0; i < size; i++) {
		fibril_mutex_lock(&shared.mutex);
		shared.counter--;
		fibril_mutex_unlock(&shared.mutex);
	}
	bench_run_stop(run);

	while (!atomic_load(&shared.done)) {
		fibril_yield();
	}

	return true;
}

benchmark_t benchmark_fibril_mutex = {
	.name = "fibril_mutex",
	.desc = "Speed of mutex lock/unlock operations",
	.entry = &runner,
	.setup = NULL,
	.teardown = NULL
};

/** @}
 */
