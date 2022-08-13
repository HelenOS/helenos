/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT

PCUT_TEST(normal_test) {
	PCUT_ASSERT_INT_EQUALS(1, 1);
}

PCUT_TEST(skipped, PCUT_TEST_SKIP) {
	PCUT_ASSERT_STR_EQUALS("skip", "not skipped");
}

PCUT_TEST(again_normal_test) {
	PCUT_ASSERT_INT_EQUALS(1, 1);
}

PCUT_MAIN()
