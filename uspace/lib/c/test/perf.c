/*
 * Copyright (c) 2018 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
