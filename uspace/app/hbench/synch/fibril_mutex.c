/*
 * SPDX-FileCopyrightText: 2019 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
