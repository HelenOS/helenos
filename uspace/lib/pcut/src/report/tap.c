/*
 * Copyright (c) 2013 Vojtech Horky
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
 * Test-anything-protocol reporting routines.
 */

#include "../internal.h"
#include "report.h"
#ifndef __helenos__
#include <string.h>
#endif
#include <stdio.h>

/** Counter of all run tests. */
static int test_counter;

/** Counter of all failures. */
static int failed_test_counter;

/** Counter for tests in a current suite. */
static int tests_in_suite;

/** Counter of failed tests in current suite. */
static int failed_tests_in_suite;

/** Initialize the TAP output.
 *
 * @param all_items Start of the list with all items.
 */
static void tap_init(pcut_item_t *all_items)
{
	int tests_total = pcut_count_tests(all_items);
	test_counter = 0;
	failed_test_counter = 0;

	printf("1..%d\n", tests_total);
}

/** Report that a suite was started.
 *
 * @param suite Suite that just started.
 */
static void tap_suite_start(pcut_item_t *suite)
{
	tests_in_suite = 0;
	failed_tests_in_suite = 0;

	printf("#> Starting suite %s.\n", suite->name);
}

/** Report that a suite was completed.
 *
 * @param suite Suite that just ended.
 */
static void tap_suite_done(pcut_item_t *suite)
{
	if (failed_tests_in_suite == 0) {
		printf("#> Finished suite %s (passed).\n",
		    suite->name);
	} else {
		printf("#> Finished suite %s (failed %d of %d).\n",
		    suite->name, failed_tests_in_suite, tests_in_suite);
	}
}

/** Report that a test was started.
 *
 * We do nothing - all handling is done after the test completes.
 *
 * @param test Test that is started.
 */
static void tap_test_start(pcut_item_t *test)
{
	PCUT_UNUSED(test);

	tests_in_suite++;
	test_counter++;
}

/** Print the buffer, prefix new line with a given string.
 *
 * @param message Message to print.
 * @param prefix Prefix for each new line, such as comment character.
 */
static void print_by_lines(const char *message, const char *prefix)
{
	char *next_line_start;
	if ((message == NULL) || (message[0] == 0)) {
		return;
	}
	next_line_start = pcut_str_find_char(message, '\n');
	while (next_line_start != NULL) {
		next_line_start[0] = 0;
		printf("%s%s\n", prefix, message);
		message = next_line_start + 1;
		next_line_start = pcut_str_find_char(message, '\n');
	}
	if (message[0] != 0) {
		printf("%s%s\n", prefix, message);
	}
}

/** Report a completed test.
 *
 * @param test Test that just finished.
 * @param outcome Outcome of the test.
 * @param error_message Buffer with error message.
 * @param teardown_error_message Buffer with error message from a tear-down function.
 * @param extra_output Extra output from the test (stdout).
 */
static void tap_test_done(pcut_item_t *test, int outcome,
    const char *error_message, const char *teardown_error_message,
    const char *extra_output)
{
	const char *test_name = test->name;
	const char *status_str = NULL;
	const char *fail_error_str = NULL;

	if (outcome != PCUT_OUTCOME_PASS) {
		failed_tests_in_suite++;
		failed_test_counter++;
	}

	switch (outcome) {
	case PCUT_OUTCOME_PASS:
		status_str = "ok";
		fail_error_str = "";
		break;
	case PCUT_OUTCOME_FAIL:
		status_str = "not ok";
		fail_error_str = " failed";
		break;
	default:
		status_str = "not ok";
		fail_error_str = " aborted";
		break;
	}
	printf("%s %d %s%s\n", status_str, test_counter, test_name, fail_error_str);

	print_by_lines(error_message, "# error: ");
	print_by_lines(teardown_error_message, "# error: ");

	print_by_lines(extra_output, "# stdio: ");
}

/** Report testing done. */
static void tap_done(void)
{
	if (failed_test_counter == 0) {
		printf("#> Done: all tests passed.\n");
	} else {
		printf("#> Done: %d of %d tests failed.\n", failed_test_counter, test_counter);
	}
}


pcut_report_ops_t pcut_report_tap = {
	tap_init, tap_done,
	tap_suite_start, tap_suite_done,
	tap_test_start, tap_test_done
};
