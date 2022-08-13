/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <str.h>
#include <pcut/pcut.h>

#define BUFFER_SIZE 8192
#define TEQ(expected, actual) PCUT_ASSERT_STR_EQUALS(expected, actual)
#define TF(expected, format, ...) TEQ(expected, fmt(format, ##__VA_ARGS__))

#define SPRINTF_TEST(test_name, expected_string, actual_format, ...) \
	PCUT_TEST(test_name) { \
		snprintf(buffer, BUFFER_SIZE, actual_format, ##__VA_ARGS__); \
		PCUT_ASSERT_STR_EQUALS(expected_string, buffer); \
	}

PCUT_INIT;

PCUT_TEST_SUITE(sprintf);

static char buffer[BUFFER_SIZE];

PCUT_TEST_BEFORE
{
	memset(buffer, 0, BUFFER_SIZE);
}

SPRINTF_TEST(no_formatting, "This is a test.", "This is a test.");

SPRINTF_TEST(string_plain, "some text", "%s", "some text");

SPRINTF_TEST(string_dynamic_width, "  tex", "%*.*s", 5, 3, "text");

SPRINTF_TEST(string_dynamic_width_align_left, "text   ", "%-*.*s", 7, 7, "text");

SPRINTF_TEST(string_pad, "    text", "%8.10s", "text");

SPRINTF_TEST(string_pad_but_cut, "  very lon", "%10.8s", "very long text");

SPRINTF_TEST(char_basic, "[a]", "[%c]", 'a');

SPRINTF_TEST(int_various_padding, "[1] [ 02] [03 ] [004] [005]",
    "[%d] [%3.2d] [%-3.2d] [%2.3d] [%-2.3d]",
    1, 2, 3, 4, 5);

SPRINTF_TEST(int_negative_various_padding, "[-1] [-02] [-03] [-004] [-005]",
    "[%d] [%3.2d] [%-3.2d] [%2.3d] [%-2.3d]",
    -1, -2, -3, -4, -5);

SPRINTF_TEST(long_negative_various_padding, "[-1] [-02] [-03] [-004] [-005]",
    "[%lld] [%3.2lld] [%-3.2lld] [%2.3lld] [%-2.3lld]",
    (long long) -1, (long long) -2, (long long) -3, (long long) -4,
    (long long) -5);

SPRINTF_TEST(int_as_hex, "[0x11] [0x012] [0x013] [0x00014] [0x00015]",
    "[%#x] [%#5.3x] [%#-5.3x] [%#3.5x] [%#-3.5x]",
    17, 18, 19, 20, 21);

PCUT_EXPORT(sprintf);
