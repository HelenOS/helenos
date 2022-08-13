/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"
#include <stdio.h>

PCUT_INIT

PCUT_TEST(print_to_stdout) {
	printf("Printed from a test to stdout!\n");
}

PCUT_TEST(print_to_stderr) {
	fprintf(stderr, "Printed from a test to stderr!\n");
}

PCUT_TEST(print_to_stdout_and_fail) {
	printf("Printed from a test to stdout!\n");
	PCUT_ASSERT_NOT_NULL(0);
}

PCUT_MAIN()
