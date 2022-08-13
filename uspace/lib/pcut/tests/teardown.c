/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT



PCUT_TEST_SUITE(with_teardown);

PCUT_TEST_AFTER {
	printf("This is teardown-function.\n");
}

PCUT_TEST(empty) {
}

PCUT_TEST(failing) {
	PCUT_ASSERT_INT_EQUALS(10, intmin(1, 2));
}



PCUT_TEST_SUITE(with_failing_teardown);

PCUT_TEST_AFTER {
	printf("This is failing teardown-function.\n");
	PCUT_ASSERT_INT_EQUALS(42, intmin(10, 20));
}

PCUT_TEST(empty2) {
}

PCUT_TEST(printing2) {
	printf("Printed before test failure.\n");
	PCUT_ASSERT_INT_EQUALS(0, intmin(-17, -19));
}

PCUT_TEST(failing2) {
	PCUT_ASSERT_INT_EQUALS(12, intmin(3, 5));
}


PCUT_MAIN()
