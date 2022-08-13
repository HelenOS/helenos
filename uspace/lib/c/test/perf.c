/*
 * SPDX-FileCopyrightText: 2018 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <fibril.h>
#include <perf.h>

PCUT_INIT;

PCUT_TEST_SUITE(perf);

/* Checks that initialization zeroes out all entries. */
PCUT_TEST(zero_diff)
{
	stopwatch_t sw;
	stopwatch_init(&sw);
	PCUT_ASSERT_INT_EQUALS(0, (int) stopwatch_get_nanos(&sw));
}

/* Checks that initialization zeroes out all entries. */
PCUT_TEST(zero_diff_static)
{
	stopwatch_t sw = STOPWATCH_INITIALIZE_STATIC;
	PCUT_ASSERT_INT_EQUALS(0, (int) stopwatch_get_nanos(&sw));
}

/* Checks that measuring 1s sleep does not give completely invalid results. */
PCUT_TEST(stopwatch_smokes)
{
	stopwatch_t sw = STOPWATCH_INITIALIZE_STATIC;
	stopwatch_start(&sw);
	fibril_sleep(1);
	stopwatch_stop(&sw);
	nsec_t diff_nanos = stopwatch_get_nanos(&sw);
	PCUT_ASSERT_TRUE(diff_nanos > MSEC2NSEC(500));
	PCUT_ASSERT_TRUE(diff_nanos < SEC2NSEC(5));
}

/* Checks that setting time works for small values. */
PCUT_TEST(stopwatch_emulation_works_small)
{
	stopwatch_t sw = STOPWATCH_INITIALIZE_STATIC;
	stopwatch_set_nanos(&sw, 42);
	PCUT_ASSERT_INT_EQUALS(42, (int) stopwatch_get_nanos(&sw));
}

/* Checks that setting time works for big values too. */
PCUT_TEST(stopwatch_emulation_works_big)
{
	stopwatch_t sw = STOPWATCH_INITIALIZE_STATIC;
	stopwatch_set_nanos(&sw, 4200000000021);
	PCUT_ASSERT_EQUALS(4200000000021, (long long) stopwatch_get_nanos(&sw));
}

PCUT_EXPORT(perf);
