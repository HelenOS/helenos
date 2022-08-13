/*
 * SPDX-FileCopyrightText: 2019 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <limits.h>
#include <types/casting.h>

PCUT_INIT;

PCUT_TEST_SUITE(casting);

/*
 * Following tests checks functionality of can_cast_size_t_to_int.
 */

PCUT_TEST(size_t_to_int_with_small)
{
	PCUT_ASSERT_TRUE(can_cast_size_t_to_int(0));
	PCUT_ASSERT_TRUE(can_cast_size_t_to_int(128));
}

PCUT_TEST(size_t_to_int_with_biggest_int)
{
	PCUT_ASSERT_TRUE(can_cast_size_t_to_int(INT_MAX));
}

PCUT_TEST(size_t_to_int_with_biggest_size_t)
{
	PCUT_ASSERT_FALSE(can_cast_size_t_to_int(SIZE_MAX));
}

PCUT_EXPORT(casting);
