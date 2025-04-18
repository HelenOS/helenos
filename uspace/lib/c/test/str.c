/*
 * Copyright (c) 2015 Michal Koutny
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

#include "pcut/asserts.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <str.h>
#include <pcut/pcut.h>

#define BUFFER_SIZE 256

#define SET_BUFFER(str) snprintf(buffer, BUFFER_SIZE, "%s", str)
#define EQ(expected, value) PCUT_ASSERT_STR_EQUALS(expected, value)

PCUT_INIT;

PCUT_TEST_SUITE(str);

static char buffer[BUFFER_SIZE];

PCUT_TEST_BEFORE
{
	memset(buffer, 0, BUFFER_SIZE);
}

/* Helper to display string contents for debugging */
static void print_string_hex(char *out, const char *s, size_t len)
{
	*out++ = '"';
	for (size_t i = 0; i < len && s[i]; i++) {
		if (s[i] >= 32 && s[i] <= 126)
			*out++ = s[i];
		else
			out += snprintf(out, 5, "\\x%02x", (uint8_t) s[i]);
	}
	*out++ = '"';
	*out++ = 0;
}

PCUT_TEST(rtrim)
{
	SET_BUFFER("foobar");
	str_rtrim(buffer, ' ');
	EQ("foobar", buffer);

	SET_BUFFER("  foobar  ");
	str_rtrim(buffer, ' ');
	EQ("  foobar", buffer);

	SET_BUFFER("  ššš  ");
	str_rtrim(buffer, ' ');
	EQ("  ššš", buffer);

	SET_BUFFER("ššAAAšš");
	str_rtrim(buffer, L'š');
	EQ("ššAAA", buffer);
}

PCUT_TEST(ltrim)
{
	SET_BUFFER("foobar");
	str_ltrim(buffer, ' ');
	EQ("foobar", buffer);

	SET_BUFFER("  foobar  ");
	str_ltrim(buffer, ' ');
	EQ("foobar  ", buffer);

	SET_BUFFER("  ššš  ");
	str_ltrim(buffer, ' ');
	EQ("ššš  ", buffer);

	SET_BUFFER("ššAAAšš");
	str_ltrim(buffer, L'š');
	EQ("AAAšš", buffer);
}

PCUT_TEST(str_str_found)
{
	const char *hs = "abracadabra";
	const char *n = "raca";
	char *p;

	p = str_str(hs, n);
	PCUT_ASSERT_TRUE((const char *)p == hs + 2);
}

PCUT_TEST(str_str_not_found)
{
	const char *hs = "abracadabra";
	const char *n = "racab";
	char *p;

	p = str_str(hs, n);
	PCUT_ASSERT_TRUE(p == NULL);
}

PCUT_TEST(str_str_empty_n)
{
	const char *hs = "abracadabra";
	const char *n = "";
	char *p;

	p = str_str(hs, n);
	PCUT_ASSERT_TRUE((const char *)p == hs);
}

PCUT_TEST(str_non_shortest)
{
	/* Overlong zero. */
	const char overlong1[] = "\xC0\x80";
	const char overlong2[] = "\xE0\x80\x80";
	const char overlong3[] = "\xF0\x80\x80\x80";

	const char overlong4[] = "\xC1\xBF";
	const char overlong5[] = "\xE0\x9F\xBF";
	const char overlong6[] = "\xF0\x8F\xBF\xBF";

	size_t offset = 0;
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, str_decode(overlong1, &offset, sizeof(overlong1)));
	offset = 0;
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, str_decode(overlong2, &offset, sizeof(overlong2)));
	offset = 0;
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, str_decode(overlong3, &offset, sizeof(overlong3)));
	offset = 0;
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, str_decode(overlong4, &offset, sizeof(overlong4)));
	offset = 0;
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, str_decode(overlong5, &offset, sizeof(overlong5)));
	offset = 0;
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, str_decode(overlong6, &offset, sizeof(overlong6)));
}

struct sanitize_test {
	const char *input;
	const char *output;
};

static const struct sanitize_test sanitize_tests[] = {
	// Empty string
	{ "", "" },
	// ASCII only
	{ "Hello, world!", "Hello, world!" },
	// Valid multi-byte sequences
	{ "Aπ你🐱", "Aπ你🐱" },
	// U+D7FF is last valid before surrogates
	{ "A\xED\x9F\xBFZ", "A\xED\x9F\xBFZ" },
	// 0x10FFFF is the highest legal code point
	{ "A\xF4\x8F\xBF\xBFZ", "A\xF4\x8F\xBF\xBFZ" },

	// Missing continuation byte
	{ "A\xC2Z", "A?Z" },
	// Truncated multi-byte at buffer end
	{ "A\xE2\x82", "A??" },
	// Continuation byte without leading byte (0x80-0xBF are never valid first bytes)
	{ "A\x80Y\xBFZ", "A?Y?Z" },

	// 'A' (U+0041) normally encoded as 0x41
	// Overlong 2-byte encoding: 0xC1 0x81
	{ "\xC1\x81X", "??X" },

	// ¢ (U+00A2) normally encoded as 0xC2 0xA2
	// Overlong 3-byte encoding: 0xE0 0x82 0xA2
	{ "\xE0\x82\xA2X", "???X" },

	// ¢ (U+00A2) normally encoded as 0xC2 0xA2
	// Overlong 4-byte encoding: 0xF0 0x80 0x82 0xA2
	{ "\xF0\x80\x82\xA2X", "????X" },

	// € (U+20AC) normally encoded as 0xE2 0x82 0xAC
	// Overlong 4-byte encoding: 0xF0 0x82 0x82 0xAC
	{ "\xF0\x82\x82\xACX", "????X" },

	// Using 0xC0 0x80 as overlong encoding for NUL (which should be just 0x00)
	{ "\xC0\x80X", "??X" },

	// 0xED 0xA0 0x80 encodes a surrogate half (U+D800), not allowed in UTF-8
	{ "A\xED\xA0\x80Z", "A???Z" },

	// 0x110000 is not a legal code point
	{ "A\xF4\x90\x80\x80Z", "A????Z" },

	// Mix of valid and invalid sequences
	{ "A\xC2\xA9\xE2\x28\xA1\xF0\x9F\x98\x81\x80Z", "A©?(?😁?Z" },
};

static size_t count_diff(const char *a, const char *b, size_t n)
{
	size_t count = 0;

	for (size_t i = 0; i < n; i++) {
		if (a[i] != b[i])
			count++;
	}

	return count;
}

PCUT_TEST(str_sanitize)
{
	char replacement = '?';
	char buffer2[255];

	for (size_t i = 0; i < sizeof(sanitize_tests) / sizeof(sanitize_tests[0]); i++) {
		const char *in = sanitize_tests[i].input;
		const char *out = sanitize_tests[i].output;
		size_t n = str_size(in) + 1;
		assert(str_size(out) + 1 == n);

		memcpy(buffer, in, n);
		size_t replaced = str_sanitize(buffer, n, replacement);
		if (memcmp(buffer, out, n) != 0) {
			print_string_hex(buffer2, buffer, n);
			print_string_hex(buffer, out, n);
			PCUT_ASSERTION_FAILED("Expected %s, got %s", buffer, buffer2);
		}

		size_t expect_replaced = count_diff(buffer, in, n);
		PCUT_ASSERT_INT_EQUALS(expect_replaced, replaced);
	}

	// Test with n smaller than string length - truncated valid encoding for €
	const char *in = "ABC€";
	const char *out = "ABC??\xAC";
	size_t n = str_size(in) + 1;
	memcpy(buffer, in, n);
	size_t replaced = str_sanitize(buffer, 5, replacement);
	if (memcmp(buffer, out, n) != 0) {
		print_string_hex(buffer2, buffer, n);
		print_string_hex(buffer, out, n);
		PCUT_ASSERTION_FAILED("Expected %s, got %s", buffer, buffer2);
	}

	PCUT_ASSERT_INT_EQUALS(2, replaced);
}

PCUT_EXPORT(str);
