/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT

PCUT_TEST_SUITE(intmin);

PCUT_TEST(test_min) {
	PCUT_ASSERT_INT_EQUALS(5, intmin(5, 654));
	PCUT_ASSERT_INT_EQUALS(5, intmin(654, 5));

	PCUT_ASSERT_INT_EQUALS(-17, intmin(-17, -2));
}

PCUT_TEST(test_same_numbers) {
	PCUT_ASSERT_INT_EQUALS(5, intmin(5, 5));
	PCUT_ASSERT_INT_EQUALS(719, intmin(719, 719));
	PCUT_ASSERT_INT_EQUALS(-4589, intmin(-4589, 4589));
}

PCUT_EXPORT(intmin_suite);
