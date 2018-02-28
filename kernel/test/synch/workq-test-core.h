/*
 * Copyright (c) 2012 Adam Hraska
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

#include <assert.h>
#include <test.h>
#include <arch.h>
#include <atomic.h>
#include <print.h>
#include <proc/thread.h>
#include <mem.h>
#include <synch/workqueue.h>


typedef struct test_work {
	work_t work_item;
	int master;
	int wave;
	int count_down;
} test_work_t;

static atomic_t call_cnt[WAVES];


/* Fwd decl - implement in your actual test file.. */
static int core_workq_enqueue(work_t *work_item, work_func_t func);


static bool new_wave(test_work_t *work)
{
	++work->wave;

	if (work->wave < WAVES) {
		work->count_down = COUNT;
		return true;
	} else {
		return false;
	}
}


static int is_pow2(int num)
{
	unsigned n = (unsigned)num;
	return (n != 0) && 0 == (n & (n-1));
}

static test_work_t * create_child(test_work_t *work)
{
	test_work_t *child = malloc(sizeof(test_work_t), 0);
	assert(child);
	if (child) {
		child->master = false;
		child->wave = work->wave;
		child->count_down = work->count_down;
	}

	return child;
}

static void free_work(test_work_t *work)
{
	memsetb(work, sizeof(test_work_t), 0xfa);
	free(work);
}

static void reproduce(work_t *work_item)
{
	/* Ensure work_item is ours for the taking. */
	memsetb(work_item, sizeof(work_t), 0xec);

	test_work_t *work = (test_work_t *)work_item;

	atomic_inc(&call_cnt[work->wave]);

	if (0 < work->count_down) {
		/* Sleep right before creating the last generation. */
		if (1 == work->count_down) {
			bool sleeping_wave = ((work->wave % 2) == 1);

			/* Master never sleeps. */
			if (sleeping_wave && !work->master) {
				thread_usleep(WAVE_SLEEP_MS * 1000);
			}
		}

		--work->count_down;

		/*
		 * Enqueue a child if count_down is power-of-2.
		 * Leads to exponential growth.
		 */
		if (is_pow2(work->count_down + 1)) {
			test_work_t *child = create_child(work);
			if (child) {
				if (!core_workq_enqueue(&child->work_item, reproduce))
					free_work(child);
			}
		}

		if (!core_workq_enqueue(work_item, reproduce)) {
			if (work->master)
				TPRINTF("\nErr: Master work item exiting prematurely!\n");

			free_work(work);
		}
	} else {
		/* We're done with this wave - only the master survives. */

		if (work->master && new_wave(work)) {
			if (!core_workq_enqueue(work_item, reproduce)) {
				TPRINTF("\nErr: Master work could not start a new wave!\n");
				free_work(work);
			}
		} else {
			if (work->master)
				TPRINTF("\nMaster work item done.\n");

			free_work(work);
		}
	}
}

static const char *run_workq_core(bool end_prematurely)
{
	for (int i = 0; i < WAVES; ++i) {
		atomic_set(&call_cnt[i], 0);
	}

	test_work_t *work = malloc(sizeof(test_work_t), 0);

	work->master = true;
	work->wave = 0;
	work->count_down = COUNT;

	/*
	 * k == COUNT_POW
	 * 2^k == COUNT + 1
	 *
	 * We have "k" branching points. Therefore:
	 * exp_call_cnt == k*2^(k-1) + 2^k == (k + 2) * 2^(k-1)
	 */
	size_t exp_call_cnt = (COUNT_POW + 2) * (1 << (COUNT_POW - 1));

	TPRINTF("waves: %d, count_down: %d, total expected calls: %zu\n",
		WAVES, COUNT, exp_call_cnt * WAVES);


	core_workq_enqueue(&work->work_item, reproduce);

	size_t sleep_cnt = 0;
	/* At least 40 seconds total (or 2 sec to end while there's work). */
	size_t max_sleep_secs = end_prematurely ? 2 : MAIN_MAX_SLEEP_SEC;
	size_t max_sleep_cnt = (max_sleep_secs * 1000) / MAIN_POLL_SLEEP_MS;

	for (int i = 0; i < WAVES; ++i) {
		while (atomic_get(&call_cnt[i]) < exp_call_cnt
			&& sleep_cnt < max_sleep_cnt) {
			TPRINTF(".");
			thread_usleep(MAIN_POLL_SLEEP_MS * 1000);
			++sleep_cnt;
		}
	}

	bool success = true;

	for (int i = 0; i < WAVES; ++i) {
		if (atomic_get(&call_cnt[i]) == exp_call_cnt) {
			TPRINTF("Ok: %" PRIua " calls in wave %d, as expected.\n",
				atomic_get(&call_cnt[i]), i);
		} else {
			success = false;
			TPRINTF("Error: %" PRIua " calls in wave %d, but %zu expected.\n",
				atomic_get(&call_cnt[i]), i, exp_call_cnt);
		}
	}


	if (success)
		return NULL;
	else {
		return "Failed to invoke the expected number of calls.\n";
	}
}
