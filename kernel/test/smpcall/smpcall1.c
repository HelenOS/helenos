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

#include <print.h>
#include <debug.h>

#include <assert.h>
#include <test.h>
#include <smp/smp_call.h>
#include <cpu.h>
#include <macros.h>
#include <config.h>
#include <arch.h>
#include <proc/thread.h>

/*
 * Maximum total number of smp_calls in the system is:
 *  162000 == 9^2 * 1000 * 2
 *  == MAX_CPUS^2 * ITERATIONS * EACH_CPU_INC_PER_ITER
 */
#define MAX_CPUS   9
#define ITERATIONS 1000
#define EACH_CPU_INC_PER_ITER 2


static void inc(void *p)
{
	assert(interrupts_disabled());

	size_t *pcall_cnt = (size_t*)p;
	/*
	 * No synchronization. Tests if smp_calls makes changes
	 * visible to the caller.
	 */
	++*pcall_cnt;
}


static void test_thread(void *p)
{
	size_t *pcall_cnt = (size_t*)p;
	smp_call_t call_info[MAX_CPUS];

	unsigned int cpu_count = min(config.cpu_active, MAX_CPUS);

	for (int iter = 0; iter < ITERATIONS; ++iter) {
		/* Synchronous version. */
		for (unsigned cpu_id = 0; cpu_id < cpu_count; ++cpu_id) {
			/*
			 * smp_call should make changes by inc() visible on this cpu.
			 * As a result we can pass it our pcall_cnt and not worry
			 * about other synchronization.
			 */
			smp_call(cpu_id, inc, pcall_cnt);
		}

		/*
		 * Async calls run in parallel on different cpus, so passing the
		 * same counter would clobber it without additional synchronization.
		 */
		size_t local_cnt[MAX_CPUS] = {0};

		/* Now start asynchronous calls. */
		for (unsigned cpu_id = 0; cpu_id < cpu_count; ++cpu_id) {
			smp_call_async(cpu_id, inc, &local_cnt[cpu_id], &call_info[cpu_id]);
		}

		/* And wait for all async calls to complete. */
		for (unsigned cpu_id = 0; cpu_id < cpu_count; ++cpu_id) {
			smp_call_wait(&call_info[cpu_id]);
			*pcall_cnt += local_cnt[cpu_id];
		}

		/* Give other threads a chance to run. */
		thread_usleep(10000);
	}
}

static size_t calc_exp_calls(size_t thread_cnt)
{
	return thread_cnt * ITERATIONS * EACH_CPU_INC_PER_ITER;
}

const char *test_smpcall1(void)
{
	/* Number of received calls that were sent by cpu[i]. */
	size_t call_cnt[MAX_CPUS] = {0};
	thread_t *thread[MAX_CPUS] = { NULL };

	unsigned int cpu_count = min(config.cpu_active, MAX_CPUS);
	size_t running_thread_cnt = 0;

	TPRINTF("Spawning threads on %u cpus.\n", cpu_count);

	/* Create a wired thread on each cpu. */
	for (unsigned int id = 0; id < cpu_count; ++id) {
		thread[id] = thread_create(test_thread, &call_cnt[id], TASK,
			THREAD_FLAG_NONE, "smp-call-test");

		if (thread[id]) {
			thread_wire(thread[id], &cpus[id]);
			++running_thread_cnt;
		} else {
			TPRINTF("Failed to create thread on cpu%u.\n", id);
		}
	}

	size_t exp_calls = calc_exp_calls(running_thread_cnt);
	size_t exp_calls_sum = exp_calls * cpu_count;

	TPRINTF("Running %zu wired threads. Expecting %zu calls. Be patient.\n",
		running_thread_cnt, exp_calls_sum);

	for (unsigned int i = 0; i < cpu_count; ++i) {
		if (thread[i] != NULL) {
			thread_ready(thread[i]);
		}
	}

	/* Wait for threads to complete. */
	for (unsigned int i = 0; i < cpu_count; ++i) {
		if (thread[i] != NULL) {
			thread_join(thread[i]);
			thread_detach(thread[i]);
		}
	}

	TPRINTF("Threads finished. Checking number of smp_call()s.\n");

	bool ok = true;
	size_t calls_sum = 0;

	for (size_t i = 0; i < cpu_count; ++i) {
		if (thread[i] != NULL) {
			if (call_cnt[i] != exp_calls) {
				ok = false;
				TPRINTF("Error: %zu instead of %zu cpu%zu's calls were"
					" acknowledged.\n", call_cnt[i], exp_calls, i);
			}
		}

		calls_sum += call_cnt[i];
	}

	if (calls_sum != exp_calls_sum) {
		TPRINTF("Error: total acknowledged sum: %zu instead of %zu.\n",
			calls_sum, exp_calls_sum);

		ok = false;
	}

	if (ok) {
		TPRINTF("Success: number of received smp_calls is as expected (%zu).\n",
			exp_calls_sum);
		return NULL;
	} else
		return "Failed: incorrect acknowledged smp_calls.\n";

}
