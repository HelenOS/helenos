/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <pcut/pcut.h>
#include <stdlib.h>
#include <stdio.h>

PCUT_INIT

static char *buffer = NULL;
#define BUFFER_SIZE 512

PCUT_TEST_SUITE(suite_with_setup_and_teardown);

PCUT_TEST_BEFORE {
	buffer = malloc(BUFFER_SIZE);
	PCUT_ASSERT_NOT_NULL(buffer);
}

PCUT_TEST_AFTER {
	free(buffer);
	buffer = NULL;
}

PCUT_TEST(test_with_setup_and_teardown) {
#if (defined(__WIN64) || defined(__WIN32) || defined(_WIN32)) && defined(_MSC_VER)
	_snprintf_s(buffer, BUFFER_SIZE - 1, _TRUNCATE, "%d-%s", 56, "abcd");
#else
	snprintf(buffer, BUFFER_SIZE - 1, "%d-%s", 56, "abcd");
#endif

	PCUT_ASSERT_STR_EQUALS("56-abcd", buffer);
}

PCUT_TEST_SUITE(another_without_setup);

PCUT_TEST(test_without_any_setup_or_teardown) {
	PCUT_ASSERT_NULL(buffer);
}


PCUT_MAIN()
