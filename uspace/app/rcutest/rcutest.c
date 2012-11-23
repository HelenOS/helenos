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
#include <mem.h>
#include <errno.h>
#include <thread.h>
#include <assert.h>
#include <async.h>
#include <fibril.h>
#include <compiler/barrier.h>

#include <rcu.h>


#define USECS_PER_SEC (1000 * 1000)
#define USECS_PER_MS  1000


typedef struct test_desc {
	/* Aggregate test that runs other tests already in the table test_desc. */
	bool aggregate;
	enum {
		T_OTHER,
		T_SANITY,
		T_STRESS
	} type;
	bool (*func)(void);
	const char *name;
	const char *desc;
} test_desc_t;


static bool run_all_tests(void);
static bool run_sanity_tests(void);
static bool run_stress_tests(void);

static bool wait_for_one_reader(void);
static bool basic_sanity_check(void);
static bool dont_wait_for_new_reader(void);
static bool wait_for_exiting_reader(void);

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
		.type = T_OTHER,
		.func = NULL,
		.name = "(null)",
		.desc = "",
	},
};

static const size_t test_desc_cnt = sizeof(test_desc) / sizeof(test_desc[0]);

/*--------------------------------------------------------------------*/

typedef int (*fibril_func_t)(void *);

static bool create_fibril(int (*func)(void*), void *arg)
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

static bool run_tests(bool (*include_filter)(test_desc_t *)) 
{
	size_t failed_cnt = 0;
	size_t ok_cnt = 0;
	
	for (size_t i = 0; i < test_desc_cnt; ++i) {
		test_desc_t *t = &test_desc[i];
		
		if (t->func && !t->aggregate && include_filter(t)) {
			printf("Running \'%s\'...\n", t->name);
			bool ok = test_desc[i].func();
			
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
static bool run_all_tests(void)
{
	printf("Running all tests...\n");
	return run_tests(all_tests_include_filter);
}

/*--------------------------------------------------------------------*/

static bool stress_tests_include_filter(test_desc_t *desc)
{
	return desc->type == T_STRESS;
}

/* Runs all available stress tests one-by-one. */
static bool run_stress_tests(void)
{
	printf("Running stress tests...\n");
	return run_tests(stress_tests_include_filter);
}

/*--------------------------------------------------------------------*/

static bool sanity_tests_include_filter(test_desc_t *desc)
{
	return desc->type == T_SANITY;
}

/* Runs all available sanity tests one-by-one. */
static bool run_sanity_tests(void)
{
	printf("Running sanity tests...\n");
	return run_tests(sanity_tests_include_filter);
}

/*--------------------------------------------------------------------*/

/* Locks/unlocks rcu and synchronizes without contention in a single fibril. */
static bool basic_sanity_check(void)
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


static int sleeping_reader(one_reader_info_t *arg)
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

static bool wait_for_one_reader(void)
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


static int preexisting_reader(two_reader_info_t *arg)
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

static int new_reader(two_reader_info_t *arg)
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

static bool dont_wait_for_new_reader(void)
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


static int exiting_locked_reader(exit_reader_info_t *arg)
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


static bool wait_for_exiting_reader(void)
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
			printf("[%u]\n", test_num);
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
	printf("Usage: rcutest [test_name|test_number]\n");
	list_tests();
	
	printf("\nExample usage:\n");
	printf("\trcutest *\n");
	printf("\trcutest sanity-tests\n");
}


int main(int argc, char **argv)
{
	rcu_register_fibril();
	
	if (argc != 2) {
		print_usage();
		
		rcu_deregister_fibril();
		return 2;
	}
	
	test_desc_t *t = find_test(argv[1]);
	
	if (t) {
		printf("Running '%s'...\n", t->name);
		bool ok = t->func();
		
		printf("%s: '%s'\n", ok ? "Passed" : "FAILED", t->name);
		
		rcu_deregister_fibril();
		return ok ? 0 : 1;
	} else {
		printf("Non-existent test name.\n");
		list_tests();
		
		rcu_deregister_fibril();
		return 3;
	}
}


/**
 * @}
 */
