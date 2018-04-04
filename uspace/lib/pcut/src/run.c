/*
 * Copyright (c) 2012-2013 Vojtech Horky
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

/** @file
 *
 * Test execution routines.
 */

#include "internal.h"
#ifndef PCUT_NO_LONG_JUMP
#include <setjmp.h>
#endif

#ifndef PCUT_NO_LONG_JUMP
/** Long-jump buffer. */
static jmp_buf start_test_jump;
#endif

/** Whether to run a tear-down function on a failure.
 *
 * Used to determine whether we are already in a tear-down context.
 */
static int execute_teardown_on_failure;

/** Whether to report test result at all.
 *
 * Used to determine whether we are the forked or the parent process.
 */
static int report_test_result;

/** Whether to print test error.
 *
 * Used to determine whether we are the forked or the parent process.
 */
static int print_test_error;

/** Whether leaving a test means a process exit. */
static int leave_means_exit;

/** Pointer to currently running test. */
static pcut_item_t *current_test = NULL;

/** Pointer to current test suite. */
static pcut_item_t *current_suite = NULL;

/** A NULL-like suite. */
static pcut_item_t default_suite;
static int default_suite_initialized = 0;

static void init_default_suite_when_needed()
{
	if (default_suite_initialized) {
		return;
	}
	default_suite.id = -1;
	default_suite.kind = PCUT_KIND_TESTSUITE;
	default_suite.previous = NULL;
	default_suite.next = NULL;
	default_suite.name = "Default";
	default_suite.setup_func = NULL;
	default_suite.teardown_func = NULL;
}

/** Find the suite given test belongs to.
 *
 * @param it The test.
 * @return Always a valid test suite item.
 */
static pcut_item_t *pcut_find_parent_suite(pcut_item_t *it)
{
	while (it != NULL) {
		if (it->kind == PCUT_KIND_TESTSUITE) {
			return it;
		}
		it = it->previous;
	}
	init_default_suite_when_needed();
	return &default_suite;
}

/** Run a set-up (tear-down) function.
 *
 * @param func Function to run (can be NULL).
 */
static void run_setup_teardown(pcut_setup_func_t func)
{
	if (func != NULL) {
		func();
	}
}

/** Terminate current test with given outcome.
 *
 * @warning This function may execute a long jump or terminate
 * current process.
 *
 * @param outcome Outcome of the current test.
 */
static void leave_test(int outcome)
{
	PCUT_DEBUG("leave_test(outcome=%d), will_exit=%s", outcome,
	    leave_means_exit ? "yes" : "no");
	if (leave_means_exit) {
		exit(outcome);
	}

#ifndef PCUT_NO_LONG_JUMP
	longjmp(start_test_jump, 1);
#endif
}

/** Process a failed assertion.
 *
 * @warning This function calls leave_test() and typically will not
 * return.
 *
 * @param message Message describing the failure.
 */
void pcut_failed_assertion(const char *message)
{
	static const char *prev_message = NULL;
	/*
	 * The assertion failed. We need to abort the current test,
	 * inform the user and perform some clean-up. That could
	 * include running the tear-down routine.
	 */
	if (print_test_error) {
		pcut_print_fail_message(message);
	}

	if (execute_teardown_on_failure) {
		execute_teardown_on_failure = 0;
		prev_message = message;
		run_setup_teardown(current_suite->teardown_func);

		/* Tear-down was okay. */
		if (report_test_result) {
			pcut_report_test_done(current_test, PCUT_OUTCOME_FAIL,
			    message, NULL, NULL);
		}
	} else {
		if (report_test_result) {
			pcut_report_test_done(current_test, PCUT_OUTCOME_FAIL,
			    prev_message, message, NULL);
		}
	}

	prev_message = NULL;

	leave_test(PCUT_OUTCOME_FAIL); /* No return. */
}

/** Run a test.
 *
 * @param test Test to execute.
 * @return Error status (zero means success).
 */
static int run_test(pcut_item_t *test)
{
	/*
	 * Set here as the returning point in case of test failure.
	 * If we get here, it means something failed during the
	 * test execution.
	 */
#ifndef PCUT_NO_LONG_JUMP
	int test_finished = setjmp(start_test_jump);
	if (test_finished) {
		return PCUT_OUTCOME_FAIL;
	}
#endif

	if (report_test_result) {
		pcut_report_test_start(test);
	}

	current_suite = pcut_find_parent_suite(test);
	current_test = test;

	pcut_hook_before_test(test);

	/*
	 * If anything goes wrong, execute the tear-down function
	 * as well.
	 */
	execute_teardown_on_failure = 1;

	/*
	 * Run the set-up function.
	 */
	run_setup_teardown(current_suite->setup_func);

	/*
	 * The setup function was performed, it is time to run
	 * the actual test.
	 */
	test->test_func();

	/*
	 * Finally, run the tear-down function. We need to clear
	 * the flag to prevent endless loop.
	 */
	execute_teardown_on_failure = 0;
	run_setup_teardown(current_suite->teardown_func);

	/*
	 * If we got here, it means everything went well with
	 * this test.
	 */
	if (report_test_result) {
		pcut_report_test_done(current_test, PCUT_OUTCOME_PASS,
		    NULL, NULL, NULL);
	}

	return PCUT_OUTCOME_PASS;
}

/** Run a test in a forked mode.
 *
 * Forked mode means that the caller of the test is already a new
 * process running this test only.
 *
 * @param test Test to execute.
 * @return Error status (zero means success).
 */
int pcut_run_test_forked(pcut_item_t *test)
{
	int rc;

	report_test_result = 0;
	print_test_error = 1;
	leave_means_exit = 1;

	rc = run_test(test);

	current_test = NULL;
	current_suite = NULL;

	return rc;
}

/** Run a test in a single mode.
 *
 * Single mode means that the test is called in the context of the
 * parent process, that is no new process is forked.
 *
 * @param test Test to execute.
 * @return Error status (zero means success).
 */
int pcut_run_test_single(pcut_item_t *test)
{
	int rc;

	report_test_result = 1;
	print_test_error = 0;
	leave_means_exit = 0;

	rc = run_test(test);

	current_test = NULL;
	current_suite = NULL;

	return rc;
}

/** Tells time-out length for a given test.
 *
 * @param test Test for which the time-out is questioned.
 * @return Timeout in seconds.
 */
int pcut_get_test_timeout(pcut_item_t *test)
{
	int timeout = PCUT_DEFAULT_TEST_TIMEOUT;
	pcut_extra_t *extras = test->extras;


	while (extras->type != PCUT_EXTRA_LAST) {
		if (extras->type == PCUT_EXTRA_TIMEOUT) {
			timeout = extras->timeout;
		}
		extras++;
	}

	return timeout;
}
