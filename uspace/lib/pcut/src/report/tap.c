/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * Test-anything-protocol reporting routines.
 */

#include "../internal.h"
#include "report.h"

#ifdef __helenos__
#define _REALLY_WANT_STRING_H
#endif

#pragma warning(push, 0)
#include <string.h>
#include <stdio.h>
#pragma warning(pop)


/** Counter of all run tests. */
static int test_counter;

/** Counter of all failures. */
static int failed_test_counter;

/** Counter for tests in a current suite. */
static int tests_in_suite;

/** Counter of failed tests in current suite. */
static int failed_tests_in_suite;

/** Comma-separated list of failed test names. */
static char *failed_test_names;

/** Initialize the TAP output.
 *
 * @param all_items Start of the list with all items.
 */
static void tap_init(pcut_item_t *all_items) {
	int tests_total = pcut_count_tests(all_items);
	test_counter = 0;
	failed_test_counter = 0;

	printf("1..%d\n", tests_total);
}

/** Report that a suite was started.
 *
 * @param suite Suite that just started.
 */
static void tap_suite_start(pcut_item_t *suite) {
	tests_in_suite = 0;
	failed_tests_in_suite = 0;

	printf("#> Starting suite %s.\n", suite->name);
}

/** Report that a suite was completed.
 *
 * @param suite Suite that just ended.
 */
static void tap_suite_done(pcut_item_t *suite) {
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
static void tap_test_start(pcut_item_t *test) {
	PCUT_UNUSED(test);

	tests_in_suite++;
	test_counter++;
}

/** Print the buffer, prefix new line with a given string.
 *
 * @param message Message to print.
 * @param prefix Prefix for each new line, such as comment character.
 */
static void print_by_lines(const char *message, const char *prefix) {
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
		const char *extra_output) {
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

	if (outcome != PCUT_OUTCOME_PASS) {
		if (failed_test_names == NULL) {
			failed_test_names = strdup(test_name);
		} else {
			char *fs = NULL;
			if (asprintf(&fs, "%s, %s",
			    failed_test_names, test_name) >= 0) {
				free(failed_test_names);
				failed_test_names = fs;
			}
		}
	}
}

/** Report testing done. */
static void tap_done(void) {
	if (failed_test_counter == 0) {
		printf("#> Done: all tests passed.\n");
	} else {
		printf("#> Done: %d of %d tests failed.\n", failed_test_counter, test_counter);
		printf("#> Failed tests: %s\n", failed_test_names);
	}
}


pcut_report_ops_t pcut_report_tap = {
	tap_init, tap_done,
	tap_suite_start, tap_suite_done,
	tap_test_start, tap_test_done
};
