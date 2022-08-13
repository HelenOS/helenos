/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT

PCUT_TEST_SUITE(intpow);

PCUT_TEST(zero_exponent) {
	PCUT_ASSERT_INT_EQUALS(1, intpow(2, 0));
}

PCUT_TEST(one_exponent) {
	PCUT_ASSERT_INT_EQUALS(2, intpow(2, 1));
	PCUT_ASSERT_INT_EQUALS(39, intpow(39, 1));
}

PCUT_TEST_SUITE(intmin);

PCUT_TEST(test_min) {
	PCUT_ASSERT_INT_EQUALS(5, intmin(5, 654));
	PCUT_ASSERT_INT_EQUALS(5, intmin(654, 5));

	PCUT_ASSERT_INT_EQUALS(-17, intmin(-17, -2));
}

PCUT_MAIN()
