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

/** @addtogroup test
 * @{
 */

/**
 * @file rcutest.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <str_error.h>
#include <mem.h>
#include <errno.h>
#include <thread.h>
#include <assert.h>
#include <async.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <compiler/barrier.h>
#include <futex.h>
#include <str.h>

#include <rcu.h>



#define USECS_PER_SEC (1000 * 1000)
#define USECS_PER_MS  1000

/* fwd decl. */
struct test_info;

typedef struct test_desc {
	/* Aggregate test that runs other tests already in the table test_desc. */
	bool aggregate;
	enum {
		T_OTHER,
		T_SANITY,
		T_STRESS
	} type;
	bool (*func)(struct test_info*);
	const char *name;
	const char *desc;
} test_desc_t;


typedef struct test_info {
	size_t thread_cnt;
	test_desc_t *desc;
} test_info_t;



static bool run_all_tests(struct test_info*);
static bool run_sanity_tests(struct test_info*);
static bool run_stress_tests(struct test_info*);

static bool wait_for_one_reader(struct test_info*);
static bool basic_sanity_check(struct test_info*);
static bool dont_wait_for_new_reader(struct test_info*);
static bool wait_for_exiting_reader(struct test_info*);
static bool seq_test(struct test_info*);


static test_desc_t test_desc[] = {
	{
		.aggregate = true,
		.type = T_OTHER,
		.func = run_all_tests,
		.name = "*",
		.desc = "Runs all tests.",
	},
	{
		.aggregate = true,
		.type = T_SANITY,
		.func = run_sanity_tests,
		.name = "sanity-tests",
		.desc = "Runs all RCU sanity tests.",
	},
	{
		.aggregate = true,
		.type = T_STRESS,
		.func = run_stress_tests,
		.name = "stress-tests",
		.desc = "Runs all RCU stress tests.",
	},

	{
		.aggregate = false,
		.type = T_SANITY,
		.func = basic_sanity_check,
		.name = "basic-sanity",
		.desc = "Locks/unlocks and syncs in 1 fibril, no contention.",
	},
	{
		.aggregate = false,
		.type = T_SANITY,
		.func = wait_for_one_reader,
		.name = "wait-for-one",
		.desc = "Syncs with one 2 secs sleeping reader.",
	},
	{
		.aggregate = false,
		.type = T_SANITY,
		.func = dont_wait_for_new_reader,
		.name = "ignore-new-r",
		.desc = "Syncs with preexisting reader; ignores new reader.",
	},
	{
		.aggregate = false,
		.type = T_SANITY,
		.func = wait_for_exiting_reader,
		.name = "dereg-unlocks",
		.desc = "Lets deregister_fibril unlock the reader section.",
	},
	{
		.aggregate = false,
		.type = T_STRESS,
		.func = seq_test,
		.name = "seq",
		.desc = "Checks lock/unlock/sync w/ global time sequence.",
	},
	{
		.aggregate = false,
		.type = T_OTHER,
		.func = NULL,
		.name = "(null)",
		.desc = "",
	},
};

static const size_t test_desc_cnt = sizeof(test_desc) / sizeof(test_desc[0]);

/*--------------------------------------------------------------------*/

static size_t next_rand(size_t seed)
{
	return (seed * 1103515245 + 12345) & ((1U << 31) - 1);
}


typedef errno_t (*fibril_func_t)(void *);

static bool create_fibril(errno_t (*func)(void*), void *arg)
{
	fid_t fid = fibril_create(func, arg);

	if (0 == fid) {
		printf("Failed to create a fibril!\n");
		return false;
	}

	fibril_add_ready(fid);
	return true;
}

/*--------------------------------------------------------------------*/

static bool run_tests(test_info_t *info, bool (*include_filter)(test_desc_t *))
{
	size_t failed_cnt = 0;
	size_t ok_cnt = 0;

	for (size_t i = 0; i < test_desc_cnt; ++i) {
		test_desc_t *t = &test_desc[i];

		if (t->func && !t->aggregate && include_filter(t)) {
			printf("Running \'%s\'...\n", t->name);
			bool ok = test_desc[i].func(info);

			if (ok) {
				++ok_cnt;
				printf("Passed: \'%s\'\n", t->name);
			} else {
				++failed_cnt;
				printf("FAILED: \'%s\'\n", t->name);
			}
		}
	}

	printf("\n");

	printf("%zu tests passed\n", ok_cnt);

	if (failed_cnt) {
		printf("%zu tests failed\n", failed_cnt);
	}

	return 0 == failed_cnt;
}

/*--------------------------------------------------------------------*/

static bool all_tests_include_filter(test_desc_t *desc)
{
	return true;
}

/* Runs all available tests tests one-by-one. */
static bool run_all_tests(test_info_t *test_info)
{
	printf("Running all tests...\n");
	return run_tests(test_info, all_tests_include_filter);
}

/*--------------------------------------------------------------------*/

static bool stress_tests_include_filter(test_desc_t *desc)
{
	return desc->type == T_STRESS;
}

/* Runs all available stress tests one-by-one. */
static bool run_stress_tests(test_info_t *test_info)
{
	printf("Running stress tests...\n");
	return run_tests(test_info, stress_tests_include_filter);
}

/*--------------------------------------------------------------------*/

static bool sanity_tests_include_filter(test_desc_t *desc)
{
	return desc->type == T_SANITY;
}

/* Runs all available sanity tests one-by-one. */
static bool run_sanity_tests(test_info_t *test_info)
{
	printf("Running sanity tests...\n");
	return run_tests(test_info, sanity_tests_include_filter);
}

/*--------------------------------------------------------------------*/

/* Locks/unlocks rcu and synchronizes without contention in a single fibril. */
static bool basic_sanity_check(test_info_t *test_info)
{
	rcu_read_lock();
	/* nop */
	rcu_read_unlock();

	rcu_read_lock();
	/* nop */
	rcu_read_unlock();

	rcu_synchronize();

	/* Nested lock with yield(). */
	rcu_read_lock();
	fibril_yield();
	rcu_read_lock();
	fibril_yield();
	rcu_read_unlock();
	fibril_yield();
	rcu_read_unlock();

	fibril_yield();
	rcu_synchronize();
	rcu_synchronize();

	rcu_read_lock();
	/* nop */
	if (!rcu_read_locked())
		return false;

	rcu_read_unlock();

	return !rcu_read_locked();
}

typedef struct one_reader_info {
	bool entered_cs;
	bool exited_cs;
	size_t done_sleeps_cnt;
	bool synching;
	bool synched;
	size_t failed;
} one_reader_info_t;


static errno_t sleeping_reader(one_reader_info_t *arg)
{
	rcu_register_fibril();

	printf("lock{");
	rcu_read_lock();
	rcu_read_lock();
	arg->entered_cs = true;
	rcu_read_unlock();

	printf("r-sleep{");
	/* 2 sec */
	async_usleep(2 * USECS_PER_SEC);
	++arg->done_sleeps_cnt;
	printf("}");

	if (arg->synched) {
		arg->failed = 1;
		printf("Error: rcu_sync exited prematurely.\n");
	}

	arg->exited_cs = true;
	rcu_read_unlock();
	printf("}");

	rcu_deregister_fibril();
	return 0;
}

static bool wait_for_one_reader(test_info_t *test_info)
{
	one_reader_info_t info = { 0 };

	if (!create_fibril((fibril_func_t) sleeping_reader, &info))
		return false;

	/* 1 sec, waits for the reader to enter its critical section and sleep. */
	async_usleep(1 * USECS_PER_SEC);

	if (!info.entered_cs || info.exited_cs) {
		printf("Error: reader is unexpectedly outside of critical section.\n");
		return false;
	}

	info.synching = true;
	printf("sync[");
	rcu_synchronize();
	printf("]\n");
	info.synched = true;

	/* Load info.exited_cs */
	memory_barrier();

	if (!info.exited_cs || info.failed) {
		printf("Error: rcu_sync() returned before the reader exited its CS.\n");
		/*
		 * Sleep some more so we don't free info on stack while the reader
		 * is using it.
		 */
		/* 1.5 sec */
		async_usleep(1500 * 1000);
		return false;
	} else {
		return true;
	}
}

/*--------------------------------------------------------------------*/

#define WAIT_STEP_US  500 * USECS_PER_MS

typedef struct two_reader_info {
	bool new_entered_cs;
	bool new_exited_cs;
	bool old_entered_cs;
	bool old_exited_cs;
	bool synching;
	bool synched;
	size_t failed;
} two_reader_info_t;


static errno_t preexisting_reader(two_reader_info_t *arg)
{
	rcu_register_fibril();

	printf("old-lock{");
	rcu_read_lock();
	arg->old_entered_cs = true;

	printf("wait-for-sync{");
	/* Wait for rcu_sync() to start waiting for us. */
	while (!arg->synching) {
		async_usleep(WAIT_STEP_US);
	}
	printf(" }");

	/* A new reader starts while rcu_sync() is in progress. */

	printf("wait-for-new-R{");
	/* Wait for the new reader to enter its reader section. */
	while (!arg->new_entered_cs) {
		async_usleep(WAIT_STEP_US);
	}
	printf(" }");

	arg->old_exited_cs = true;

	assert(!arg->new_exited_cs);

	if (arg->synched) {
		arg->failed = 1;
		printf("Error: rcu_sync() did not wait for preexisting reader.\n");
	}

	rcu_read_unlock();
	printf(" }");

	rcu_deregister_fibril();
	return 0;
}

static errno_t new_reader(two_reader_info_t *arg)
{
	rcu_register_fibril();

	/* Wait until rcu_sync() starts. */
	while (!arg->synching) {
		async_usleep(WAIT_STEP_US);
	}

	/*
	 * synching is set when rcu_sync() is about to be entered so wait
	 * some more to make sure it really does start executing.
	 */
	async_usleep(WAIT_STEP_US);

	printf("new-lock(");
	rcu_read_lock();
	arg->new_entered_cs = true;

	/* Wait for rcu_sync() exit, ie stop waiting for the preexisting reader. */
	while (!arg->synched) {
		async_usleep(WAIT_STEP_US);
	}

	arg->new_exited_cs = true;
	/* Write new_exited_cs before exiting reader section. */
	memory_barrier();

	/*
	 * Preexisting reader should have exited by now, so rcu_synchronize()
	 * must have returned.
	 */
	if (!arg->old_exited_cs) {
		arg->failed = 1;
		printf("Error: preexisting reader should have exited by now!\n");
	}

	rcu_read_unlock();
	printf(")");

	rcu_deregister_fibril();
	return 0;
}

static bool dont_wait_for_new_reader(test_info_t *test_info)
{
	two_reader_info_t info = { 0 };

	if (!create_fibril((fibril_func_t) preexisting_reader, &info))
		return false;

	if (!create_fibril((fibril_func_t) new_reader, &info))
		return false;

	/* Waits for the preexisting_reader to enter its CS.*/
	while (!info.old_entered_cs) {
		async_usleep(WAIT_STEP_US);
	}

	assert(!info.old_exited_cs);
	assert(!info.new_entered_cs);
	assert(!info.new_exited_cs);

	printf("sync[");
	info.synching = true;
	rcu_synchronize();
	printf(" ]");

	/* Load info.exited_cs */
	memory_barrier();

	if (!info.old_exited_cs) {
		printf("Error: rcu_sync() returned before preexisting reader exited.\n");
		info.failed = 1;
	}

	bool new_outside_cs = !info.new_entered_cs || info.new_exited_cs;

	/* Test if new reader is waiting in CS before setting synched. */
	compiler_barrier();
	info.synched = true;

	if (new_outside_cs) {
		printf("Error: new reader CS held up rcu_sync(). (4)\n");
		info.failed = 1;
	} else {
		/* Wait for the new reader. */
		rcu_synchronize();

		if (!info.new_exited_cs) {
			printf("Error: 2nd rcu_sync() returned before new reader exited.\n");
			info.failed = 1;
		}

		printf("\n");
	}

	if (info.failed) {
		/*
		 * Sleep some more so we don't free info on stack while readers
		 * are using it.
		 */
		async_usleep(WAIT_STEP_US);
	}

	return 0 == info.failed;
}

#undef WAIT_STEP_US

/*--------------------------------------------------------------------*/
#define WAIT_STEP_US  500 * USECS_PER_MS

typedef struct exit_reader_info {
	bool entered_cs;
	bool exited_cs;
	bool synching;
	bool synched;
} exit_reader_info_t;


static errno_t exiting_locked_reader(exit_reader_info_t *arg)
{
	rcu_register_fibril();

	printf("old-lock{");
	rcu_read_lock();
	rcu_read_lock();
	rcu_read_lock();
	arg->entered_cs = true;

	printf("wait-for-sync{");
	/* Wait for rcu_sync() to start waiting for us. */
	while (!arg->synching) {
		async_usleep(WAIT_STEP_US);
	}
	printf(" }");

	rcu_read_unlock();
	printf(" }");

	arg->exited_cs = true;
	/* Store exited_cs before unlocking reader section in deregister. */
	memory_barrier();

	/* Deregister forcefully unlocks the reader section. */
	rcu_deregister_fibril();
	return 0;
}


static bool wait_for_exiting_reader(test_info_t *test_info)
{
	exit_reader_info_t info = { 0 };

	if (!create_fibril((fibril_func_t) exiting_locked_reader, &info))
		return false;

	/* Waits for the preexisting_reader to enter its CS.*/
	while (!info.entered_cs) {
		async_usleep(WAIT_STEP_US);
	}

	assert(!info.exited_cs);

	printf("sync[");
	info.synching = true;
	rcu_synchronize();
	info.synched = true;
	printf(" ]\n");

	/* Load info.exited_cs */
	memory_barrier();

	if (!info.exited_cs) {
		printf("Error: rcu_deregister_fibril did not unlock the CS.\n");
		return false;
	}

	return true;
}

#undef WAIT_STEP_US


/*--------------------------------------------------------------------*/

typedef struct {
	atomic_t time;
	atomic_t max_start_time_of_done_sync;

	size_t total_workers;
	size_t done_reader_cnt;
	size_t done_updater_cnt;
	fibril_mutex_t done_cnt_mtx;
	fibril_condvar_t done_cnt_changed;

	size_t read_iters;
	size_t upd_iters;

	atomic_t seed;
	int failed;
} seq_test_info_t;


static void signal_seq_fibril_done(seq_test_info_t *arg, size_t *cnt)
{
	fibril_mutex_lock(&arg->done_cnt_mtx);
	++*cnt;

	if (arg->total_workers == arg->done_reader_cnt + arg->done_updater_cnt) {
		fibril_condvar_signal(&arg->done_cnt_changed);
	}

	fibril_mutex_unlock(&arg->done_cnt_mtx);
}

static errno_t seq_reader(seq_test_info_t *arg)
{
	rcu_register_fibril();

	size_t seed = (size_t) atomic_preinc(&arg->seed);
	bool first = (seed == 1);

	for (size_t k = 0; k < arg->read_iters; ++k) {
		/* Print progress if the first reader fibril. */
		if (first && 0 == k % (arg->read_iters/100 + 1)) {
			printf(".");
		}

		rcu_read_lock();
		atomic_count_t start_time = atomic_preinc(&arg->time);

		/* Do some work. */
		seed = next_rand(seed);
		size_t idle_iters = seed % 8;

		for (size_t i = 0; i < idle_iters; ++i) {
			fibril_yield();
		}

		/*
		 * Check if the most recently started rcu_sync of the already
		 * finished rcu_syncs did not happen to start after this reader
		 * and, therefore, should have waited for this reader to exit
		 * (but did not - since it already announced it completed).
		 */
		if (start_time <= atomic_get(&arg->max_start_time_of_done_sync)) {
			arg->failed = 1;
		}

		rcu_read_unlock();
	}

	rcu_deregister_fibril();

	signal_seq_fibril_done(arg, &arg->done_reader_cnt);
	return 0;
}

static errno_t seq_updater(seq_test_info_t *arg)
{
	rcu_register_fibril();

	for (size_t k = 0; k < arg->upd_iters; ++k) {
		atomic_count_t start_time = atomic_get(&arg->time);
		rcu_synchronize();

		/* This is prone to a race but if it happens it errs to the safe side.*/
		if (atomic_get(&arg->max_start_time_of_done_sync) < start_time) {
			atomic_set(&arg->max_start_time_of_done_sync, start_time);
		}
	}

	rcu_deregister_fibril();

	signal_seq_fibril_done(arg, &arg->done_updater_cnt);
	return 0;
}

static bool seq_test(test_info_t *test_info)
{
	size_t reader_cnt = test_info->thread_cnt;
	size_t updater_cnt = test_info->thread_cnt;

	seq_test_info_t info = {
		.time = {0},
		.max_start_time_of_done_sync = {0},
		.read_iters = 10 * 1000,
		.upd_iters = 5 * 1000,
		.total_workers = updater_cnt + reader_cnt,
		.done_reader_cnt = 0,
		.done_updater_cnt = 0,
		.done_cnt_mtx = FIBRIL_MUTEX_INITIALIZER(info.done_cnt_mtx),
		.done_cnt_changed = FIBRIL_CONDVAR_INITIALIZER(info.done_cnt_changed),
		.seed = {0},
		.failed = 0,
	};

	/* Create and start worker fibrils. */
	for (size_t k = 0; k + k < reader_cnt + updater_cnt; ++k) {
		bool ok = create_fibril((fibril_func_t) seq_reader, &info);
		ok = ok && create_fibril((fibril_func_t) seq_updater, &info);

		if (!ok) {
			/* Let the already created fibrils corrupt the stack. */
			return false;
		}
	}

	/* Wait for all worker fibrils to complete their work. */
	fibril_mutex_lock(&info.done_cnt_mtx);

	while (info.total_workers != info.done_reader_cnt + info.done_updater_cnt) {
		fibril_condvar_wait(&info.done_cnt_changed, &info.done_cnt_mtx);
	}

	fibril_mutex_unlock(&info.done_cnt_mtx);

	if (info.failed) {
		printf("Error: rcu_sync() did not wait for a preexisting reader.");
	}

	return 0 == info.failed;
}

/*--------------------------------------------------------------------*/

static FIBRIL_MUTEX_INITIALIZE(blocking_mtx);

static void dummy_fibril(void *arg)
{
	/* Block on an already locked mutex - enters the fibril manager. */
	fibril_mutex_lock(&blocking_mtx);
	assert(false);
}

static bool create_threads(size_t cnt)
{
	/* Sanity check. */
	assert(cnt < 1024);

	/* Keep this mutex locked so that dummy fibrils never exit. */
	bool success = fibril_mutex_trylock(&blocking_mtx);
	assert(success);

	for (size_t k = 0; k < cnt; ++k) {
		thread_id_t tid;

		errno_t ret = thread_create(dummy_fibril, NULL, "urcu-test-worker", &tid);
		if (EOK != ret) {
			printf("Failed to create thread '%zu' (error: %s)\n", k + 1, str_error_name(ret));
			return false;
		}
	}

	return true;
}

/*--------------------------------------------------------------------*/
static test_desc_t *find_test(const char *name)
{
	/* First match for test name. */
	for (size_t k = 0; k < test_desc_cnt; ++k) {
		test_desc_t *t = &test_desc[k];

		if (t->func && 0 == str_cmp(t->name, name))
			return t;
	}

	/* Try to match the test number. */
	uint32_t test_num = 0;

	if (EOK == str_uint32_t(name, NULL, 0, true, &test_num)) {
		if (test_num < test_desc_cnt && test_desc[test_num].func) {
			return &test_desc[test_num];
		}
	}

	return NULL;
}

static void list_tests(void)
{
	printf("Available tests: \n");

	for (size_t i = 0; i < test_desc_cnt; ++i) {
		test_desc_t *t = &test_desc[i];

		if (!t->func)
			continue;

		const char *type = "";

		if (t->type == T_SANITY)
			type = " (sanity)";
		if (t->type == T_STRESS)
			type = " (stress)";

		printf("%zu: %s ..%s %s\n", i, t->name, type, t->desc);
	}
}


static void print_usage(void)
{
	printf("Usage: rcutest [test_name|test_number] {number_of_threads}\n");
	list_tests();

	printf("\nExample usage:\n");
	printf("\trcutest *\n");
	printf("\trcutest sanity-tests\n");
}


static bool parse_cmd_line(int argc, char **argv, test_info_t *info)
{
	if (argc != 2 && argc != 3) {
		print_usage();
		return false;
	}

	info->desc = find_test(argv[1]);

	if (!info->desc) {
		printf("Non-existent test '%s'.\n", argv[1]);
		list_tests();
		return false;
	}

	if (argc == 3) {
		uint32_t thread_cnt = 0;
		errno_t ret = str_uint32_t(argv[2], NULL, 0, true, &thread_cnt);

		if (ret == EOK && 1 <= thread_cnt && thread_cnt <= 64) {
			info->thread_cnt = thread_cnt;
		} else {
			info->thread_cnt = 1;
			printf("Err: Invalid number of threads '%s'; using 1.\n", argv[2]);
		}
	} else {
		info->thread_cnt = 1;
	}

	return true;
}

int main(int argc, char **argv)
{
	rcu_register_fibril();

	test_info_t info;

	bool ok = parse_cmd_line(argc, argv, &info);
	ok = ok && create_threads(info.thread_cnt - 1);

	if (ok) {
		assert(1 <= info.thread_cnt);
		test_desc_t *t = info.desc;

		printf("Running '%s' (in %zu threads)...\n", t->name, info.thread_cnt);
		ok = t->func(&info);

		printf("%s: '%s'\n", ok ? "Passed" : "FAILED", t->name);

		rcu_deregister_fibril();

		/* Let the kernel clean up the created background threads. */
		return ok ? 0 : 1;
	} else {
		rcu_deregister_fibril();
		return 2;
	}
}


/**
 * @}
 */
