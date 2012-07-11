/*
 */

#include <print.h>
#include <debug.h>

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
	ASSERT(interrupts_disabled());

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
	thread_t *thread[MAX_CPUS] = {0};
	
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
				TPRINTF("Error: %zu instead of %zu cpu%u's calls were"
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
