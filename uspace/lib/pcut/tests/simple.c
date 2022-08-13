/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT

PCUT_TEST(zero_exponent) {
	PCUT_ASSERT_INT_EQUALS(1, intpow(2, 0));
}

PCUT_TEST(one_exponent) {
	PCUT_ASSERT_INT_EQUALS(2, intpow(2, 1));
	PCUT_ASSERT_INT_EQUALS(39, intpow(39, 1));
}

PCUT_TEST(same_strings) {
	const char *p = "xyz";
	PCUT_ASSERT_STR_EQUALS("xyz", p);
	PCUT_ASSERT_STR_EQUALS("abc", "XXXabd" + 3);
}

PCUT_MAIN()
