/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <pcut/pcut.h>
#include <stdlib.h>

PCUT_INIT

PCUT_TEST_AFTER {
	abort();
}

PCUT_TEST(print_and_fail) {
	printf("Tear-down will cause null pointer access...\n");
	PCUT_ASSERT_NOT_NULL(NULL);
}

PCUT_MAIN()
