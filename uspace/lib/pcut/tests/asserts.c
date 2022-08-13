/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT

PCUT_TEST(int_equals) {
	PCUT_ASSERT_INT_EQUALS(1, 1);
	PCUT_ASSERT_INT_EQUALS(1, 0);
}

PCUT_TEST(double_equals) {
	PCUT_ASSERT_DOUBLE_EQUALS(1., 1., 0.001);
	PCUT_ASSERT_DOUBLE_EQUALS(1., 0.5, 0.4);
}

PCUT_TEST(str_equals) {
	PCUT_ASSERT_STR_EQUALS("xyz", "xyz");
	PCUT_ASSERT_STR_EQUALS("abc", "xyz");
}

PCUT_TEST(str_equals_or_null_base) {
	PCUT_ASSERT_STR_EQUALS_OR_NULL("xyz", "xyz");
}

PCUT_TEST(str_equals_or_null_different) {
	PCUT_ASSERT_STR_EQUALS_OR_NULL("abc", "xyz");
}

PCUT_TEST(str_equals_or_null_one_null) {
	PCUT_ASSERT_STR_EQUALS_OR_NULL(NULL, "xyz");
}

PCUT_TEST(str_equals_or_null_both) {
	PCUT_ASSERT_STR_EQUALS_OR_NULL(NULL, NULL);
}

PCUT_TEST(assert_true) {
	PCUT_ASSERT_TRUE(42);
	PCUT_ASSERT_TRUE(0);
}

PCUT_TEST(assert_false) {
	PCUT_ASSERT_FALSE(0);
	PCUT_ASSERT_FALSE(42);
}

PCUT_MAIN()
