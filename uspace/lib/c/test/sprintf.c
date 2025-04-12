/*
 * Copyright (c) 2014 Vojtech Horky
 * Copyright (c) 2025 Jiří Zárevúcky
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

#include <stdio.h>
#include <str.h>
#include <pcut/pcut.h>

#pragma GCC diagnostic ignored "-Wformat"

#define BUFFER_SIZE 8192
#define TEQ(expected, actual) PCUT_ASSERT_STR_EQUALS(expected, actual)
#define TF(expected, format, ...) TEQ(expected, fmt(format, ##__VA_ARGS__))

#define SPRINTF_TEST(test_name, expected_string, actual_format, ...) \
	PCUT_TEST(printf_##test_name) { \
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

SPRINTF_TEST(int_as_hex, "[1a] [  02b] [03c  ] [    04d] [05e    ] [0006f] [00080]",
    "[%x] [%5.3x] [%-5.3x] [%7.3x] [%-7.3x] [%3.5x] [%-3.5x]",
    26, 43, 60, 77, 94, 111, 128);

SPRINTF_TEST(int_as_hex_alt, "[0x1a] [0x02b] [0x03c] [  0x04d] [0x05e  ] [0x0006f] [0x00080]",
    "[%#x] [%#5.3x] [%#-5.3x] [%#7.3x] [%#-7.3x] [%#3.5x] [%#-3.5x]",
    26, 43, 60, 77, 94, 111, 128);

SPRINTF_TEST(int_as_hex_uc, "[1A] [  02B] [03C  ] [    04D] [05E    ] [0006F] [00080]",
    "[%X] [%5.3X] [%-5.3X] [%7.3X] [%-7.3X] [%3.5X] [%-3.5X]",
    26, 43, 60, 77, 94, 111, 128);

SPRINTF_TEST(int_as_hex_alt_uc, "[0X1A] [0X02B] [0X03C] [  0X04D] [0X05E  ] [0X0006F] [0X00080]",
    "[%#X] [%#5.3X] [%#-5.3X] [%#7.3X] [%#-7.3X] [%#3.5X] [%#-3.5X]",
    26, 43, 60, 77, 94, 111, 128);

SPRINTF_TEST(max_negative, "-9223372036854775808", "%" PRId64, INT64_MIN);

SPRINTF_TEST(sign1, "[12] [ 12] [+12] [+12] [+12] [+12]",   "[%d] [% d] [%+d] [% +d] [%+ d] [%++ ++    +  ++++d]", 12, 12, 12, 12, 12, 12);
SPRINTF_TEST(sign2, "[-12] [-12] [-12] [-12] [-12] [-12]", "[%d] [% d] [%+d] [% +d] [%+ d] [%++ ++    +  ++++d]", -12, -12, -12, -12, -12, -12);

/* When zero padding and precision and/or left justification are both specified, zero padding is ignored. */
SPRINTF_TEST(zero_left_padding, "[    0012] [0034    ] [56      ]", "[%08.4d] [%-08.4d] [%-08d]", 12, 34, 56);

/* Zero padding comes after the sign, but space padding doesn't. */
SPRINTF_TEST(sign_padding, "[00012] [   12] [ 0012] [   12] [+0012] [  +12]", "[%05d] [%5d] [%0 5d] [% 5d] [%0+5d] [%+5d]", 12, 12, 12, 12, 12, 12);
SPRINTF_TEST(sign_padding2, "[-0012] [  -12] [-0012] [  -12] [-0012] [  -12]", "[%05d] [%5d] [%0 5d] [% 5d] [%0+5d] [%+5d]", -12, -12, -12, -12, -12, -12);

SPRINTF_TEST(all_zero, "[00000] [0] [0] [0] [0] [0] [0] [0] [0]", "[%05d] [%d] [%x] [%#x] [%o] [%#o] [%b] [%#b] [%u]", 0, 0, 0, 0, 0, 0, 0, 0, 0);

PCUT_EXPORT(sprintf);
