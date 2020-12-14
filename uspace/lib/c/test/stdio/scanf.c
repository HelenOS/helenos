/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Test formatted input (scanf family)
 */

#include <mem.h>
#include <pcut/pcut.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wformat-zero-length"
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

PCUT_INIT;

PCUT_TEST_SUITE(scanf);

enum {
	chars_size = 10
};

/** Empty format string */
PCUT_TEST(empty_fmt)
{
	int rc;

	rc = sscanf("42", "");
	PCUT_ASSERT_INT_EQUALS(0, rc);
}

/** Decimal integer */
PCUT_TEST(dec_int)
{
	int rc;
	int i;

	rc = sscanf("42", "%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Two integers */
PCUT_TEST(int_int)
{
	int rc;
	int i, j;

	rc = sscanf("42 43", "%d%d", &i, &j);
	PCUT_ASSERT_INT_EQUALS(2, rc);
	PCUT_ASSERT_TRUE(i == 42);
	PCUT_ASSERT_TRUE(j == 43);
}

/** Decimal signed char */
PCUT_TEST(dec_sign_char)
{
	int rc;
	signed char sc;

	rc = sscanf("42", "%hhd", &sc);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(sc == 42);
}

/** Decimal short */
PCUT_TEST(dec_short)
{
	int rc;
	short si;

	rc = sscanf("42", "%hd", &si);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(si == 42);
}

/** Decimal long */
PCUT_TEST(dec_long)
{
	int rc;
	long li;

	rc = sscanf("42", "%ld", &li);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(li == 42);
}

/** Decimal long long */
PCUT_TEST(dec_long_long)
{
	int rc;
	long long lli;

	rc = sscanf("42", "%lld", &lli);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(lli == 42);
}

/** Decimal intmax_t */
PCUT_TEST(dec_intmax)
{
	int rc;
	intmax_t imax;

	rc = sscanf("42", "%jd", &imax);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(imax == 42);
}

/** Decimal size_t-sized */
PCUT_TEST(dec_size_t_size)
{
	int rc;
	size_t szi;

	rc = sscanf("42", "%zd", &szi);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(szi == 42);
}

/** Decimal ptrdiff_t-sized */
PCUT_TEST(dec_ptrdiff_t_size)
{
	int rc;
	ptrdiff_t pdi;

	rc = sscanf("42", "%td", &pdi);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(pdi == 42);
}

/** Decimal integer followed by hexadecimal digit */
PCUT_TEST(dec_int_hexdigit)
{
	int rc;
	int i;

	rc = sscanf("42a", "%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Decimal integer - detect no prefix */
PCUT_TEST(int_noprefix)
{
	int rc;
	int i;

	rc = sscanf("42", "%i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Prefixed octal integer followed by decimal digit */
PCUT_TEST(octal_decimal_digit)
{
	int rc;
	int i;

	rc = sscanf("019", "%i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 1);
}

/** Prefixed hexadecimal integer followed by other character */
PCUT_TEST(hex_other_char)
{
	int rc;
	int i;

	rc = sscanf("0xag", "%i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 10);
}

/** Decimal integer with '+' sign */
PCUT_TEST(positive_dec)
{
	int rc;
	int i;

	rc = sscanf("+42", "%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Decimal integer with '-' sign */
PCUT_TEST(negative_dec)
{
	int rc;
	int i;

	rc = sscanf("-42", "%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == -42);
}

/** Hexadecimal integer with prefix and '-' sign */
PCUT_TEST(negative_hex)
{
	int rc;
	int i;

	rc = sscanf("-0xa", "%i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == -10);
}

/** Decimal unsigned integer */
PCUT_TEST(dec_unsigned)
{
	int rc;
	unsigned u;

	rc = sscanf("42", "%u", &u);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(u == 42);
}

/** Decimal unsigned char */
PCUT_TEST(dec_unsigned_char)
{
	int rc;
	unsigned char uc;

	rc = sscanf("42", "%hhu", &uc);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(uc == 42);
}

/** Decimal unsigned short */
PCUT_TEST(dec_unsigned_short)
{
	int rc;
	unsigned short su;

	rc = sscanf("42", "%hu", &su);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(su == 42);
}

/** Decimal unsigned long */
PCUT_TEST(dec_unsigned_long)
{
	int rc;
	unsigned long lu;

	rc = sscanf("42", "%lu", &lu);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(lu == 42);
}

/** Decimal unsigned long long */
PCUT_TEST(dec_unsigned_long_long)
{
	int rc;
	unsigned long long llu;

	rc = sscanf("42", "%llu", &llu);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(llu == 42);
}

/** Decimal uintmax_t */
PCUT_TEST(dec_unitmax)
{
	int rc;
	uintmax_t umax;

	rc = sscanf("42", "%ju", &umax);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(umax == 42);
}

/** Decimal size_t */
PCUT_TEST(dec_unsigned_size)
{
	int rc;
	size_t szu;

	rc = sscanf("42", "%zu", &szu);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(szu == 42);
}

/** Decimal ptrdiff_t-sized unsigned int */
PCUT_TEST(dec_unsigned_ptrdiff)
{
	int rc;
	ptrdiff_t pdu;

	rc = sscanf("42", "%tu", &pdu);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(pdu == 42);
}

/** Octal unsigned integer */
PCUT_TEST(octal_unsigned)
{
	int rc;
	unsigned u;

	rc = sscanf("52", "%o", &u);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(u == 052);
}

/** Hexadecimal unsigned integer */
PCUT_TEST(hex_unsigned)
{
	int rc;
	unsigned u;

	rc = sscanf("2a", "%x", &u);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(u == 0x2a);
}

/** Hexadecimal unsigned integer unsing alternate specifier */
PCUT_TEST(hex_unsigned_cap_x)
{
	int rc;
	unsigned u;

	rc = sscanf("2a", "%X", &u);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(u == 0x2a);
}

/** Uppercase hexadecimal unsigned integer */
PCUT_TEST(uppercase_hex_unsigned)
{
	int rc;
	unsigned u;

	rc = sscanf("2A", "%x", &u);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(u == 0x2a);
}

/** Make sure %x does not match 0x prefix */
PCUT_TEST(hex_not_match_0x)
{
	int rc;
	unsigned u;

	rc = sscanf("0x1", "%x", &u);

	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(u == 0);
}

/** Skipping whitespace */
PCUT_TEST(skipws)
{
	int rc;
	int i;

	rc = sscanf(" \t\n42", "%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Percentile conversion */
PCUT_TEST(percentile)
{
	int rc;
	int i;

	rc = sscanf(" \t\n%42", "%%%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Matching specific character */
PCUT_TEST(match_spec_char)
{
	int rc;
	int i;

	rc = sscanf("x42", "x%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Matching specific character should not skip whitespace */
PCUT_TEST(match_char_noskipws)
{
	int rc;
	int i;

	rc = sscanf(" x42", "x%d", &i);
	PCUT_ASSERT_INT_EQUALS(0, rc);
}

/** Skipping whitespace + match specific character */
PCUT_TEST(skipws_match_char)
{
	int rc;
	int i;

	rc = sscanf(" x42", "\t\nx%d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Decimal with limited, but sufficient width */
PCUT_TEST(dec_sufficient_lim_width)
{
	int rc;
	int i;

	rc = sscanf("42", "%2d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 42);
}

/** Decimal with limited, smaller width */
PCUT_TEST(dec_smaller_width)
{
	int rc;
	int i;

	rc = sscanf("42", "%1d", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 4);
}

/** Integer with hex prefix, format with limited, sufficient width */
PCUT_TEST(int_hex_limited_width)
{
	int rc;
	int i;

	rc = sscanf("0x1", "%3i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 1);
}

/** Integer with hex prefix, format with limited, smaller width */
PCUT_TEST(int_hex_small_width)
{
	int rc;
	int i;

	rc = sscanf("0x1", "%2i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 0);
}

/** Integer with octal prefix, format with limited, sufficient width */
PCUT_TEST(int_oct_limited_width)
{
	int rc;
	int i;

	rc = sscanf("012", "%3i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 012);
}

/** Integer with octal prefix, format with limited, smaller width */
PCUT_TEST(int_oct_smaller_width)
{
	int rc;
	int i;

	rc = sscanf("012", "%2i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 01);
}

/** Integer with octal prefix, format with width allowing just for 0 */
PCUT_TEST(int_oct_tiny_width)
{
	int rc;
	int i;

	rc = sscanf("012", "%1i", &i);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(i == 0);
}

/** Pointer */
PCUT_TEST(pointer)
{
	int rc;
	void *ptr;

	rc = sscanf("0xABCDEF88", "%p", &ptr);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(ptr == (void *)0xABCDEF88);
}

/** Single character */
PCUT_TEST(single_char)
{
	int rc;
	char c;

	rc = sscanf("x", "%c", &c);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(c == 'x');
}

/** Single whitespace character */
PCUT_TEST(single_ws_char)
{
	int rc;
	char c;

	rc = sscanf("\t", "%c", &c);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(c == '\t');
}

/** Multiple characters */
PCUT_TEST(chars)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abc", "%3c", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == 'X');
}

/** Fewer characters than requested */
PCUT_TEST(fewer_chars)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abc", "%5c", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == 'X');
}

/** Reading characters but no found */
PCUT_TEST(chars_not_found)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("", "%5c", chars);
	PCUT_ASSERT_INT_EQUALS(EOF, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'X');
}

/** Multiple characters with suppressed assignment */
PCUT_TEST(chars_noassign)
{
	int rc;
	int n;

	rc = sscanf("abc", "%*3c%n", &n);
	PCUT_ASSERT_INT_EQUALS(0, rc);
	PCUT_ASSERT_INT_EQUALS(3, n);
}

/** Multiple characters with memory allocation */
PCUT_TEST(chars_malloc)
{
	int rc;
	char *cp;

	cp = NULL;
	rc = sscanf("abc", "%m3c", &cp);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_NOT_NULL(cp);
	PCUT_ASSERT_TRUE(cp[0] == 'a');
	PCUT_ASSERT_TRUE(cp[1] == 'b');
	PCUT_ASSERT_TRUE(cp[2] == 'c');
	free(cp);
}

/** String of non-whitespace characters, unlimited width */
PCUT_TEST(str)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf(" abc d", "%s", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** String of non-whitespace characters, until the end */
PCUT_TEST(str_till_end)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf(" abc", "%s", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** String of non-whitespace characters, large enough width */
PCUT_TEST(str_large_width)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf(" abc d", "%5s", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Want string of non-whitespace, but got only whitespace */
PCUT_TEST(str_not_found)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf(" ", "%s", chars);
	PCUT_ASSERT_INT_EQUALS(EOF, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'X');
}

/** String of non-whitespace characters, small width */
PCUT_TEST(str_small_width)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf(" abc", "%2s", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == '\0');
	PCUT_ASSERT_TRUE(chars[3] == 'X');
}

/** String of non-whitespace characters, assignment suppression */
PCUT_TEST(str_noassign)
{
	int rc;
	int n;

	rc = sscanf(" abc d", "%*s%n", &n);
	PCUT_ASSERT_INT_EQUALS(0, rc);
	PCUT_ASSERT_INT_EQUALS(4, n);
}

/** String of non-whitespace characters, memory allocation */
PCUT_TEST(str_malloc)
{
	int rc;
	char *cp;

	rc = sscanf(" abc d", "%ms", &cp);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_NOT_NULL(cp);
	PCUT_ASSERT_TRUE(cp[0] == 'a');
	PCUT_ASSERT_TRUE(cp[1] == 'b');
	PCUT_ASSERT_TRUE(cp[2] == 'c');
	PCUT_ASSERT_TRUE(cp[3] == '\0');
	free(cp);
}

/** Set conversion without width specified terminating before the end  */
PCUT_TEST(set_convert)
{
	int rc;
	char chars[chars_size];
	int i;

	memset(chars, 'X', chars_size);
	rc = sscanf("abcd42", "%[abc]d%d", chars, &i);
	PCUT_ASSERT_INT_EQUALS(2, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
	PCUT_ASSERT_TRUE(i == 42);
}

/** Set conversion without width specified, until the end */
PCUT_TEST(set_till_end)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abc", "%[abc]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with larger width */
PCUT_TEST(set_large_width)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abcd", "%5[abc]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with smaller width */
PCUT_TEST(set_small_width)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abcd", "%3[abcd]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with negated scanset */
PCUT_TEST(set_inverted)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abcd", "%[^d]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with ']' in scanset */
PCUT_TEST(set_with_rbr)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("]bcd", "%[]bc]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == ']');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with ']' in inverted scanset */
PCUT_TEST(set_inverted_with_rbr)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abc]", "%[^]def]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with leading '-' in scanset */
PCUT_TEST(set_with_leading_dash)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("a-bc[", "%[-abc]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == '-');
	PCUT_ASSERT_TRUE(chars[2] == 'b');
	PCUT_ASSERT_TRUE(chars[3] == 'c');
	PCUT_ASSERT_TRUE(chars[4] == '\0');
	PCUT_ASSERT_TRUE(chars[5] == 'X');
}

/** Set conversion with trailing '-' in scanset */
PCUT_TEST(set_with_trailing_dash)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("a-bc]", "%[abc-]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == '-');
	PCUT_ASSERT_TRUE(chars[2] == 'b');
	PCUT_ASSERT_TRUE(chars[3] == 'c');
	PCUT_ASSERT_TRUE(chars[4] == '\0');
	PCUT_ASSERT_TRUE(chars[5] == 'X');
}

/** Set conversion with leading '-' in inverted scanset */
PCUT_TEST(set_inverted_with_leading_dash)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("def-", "%[^-abc]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'd');
	PCUT_ASSERT_TRUE(chars[1] == 'e');
	PCUT_ASSERT_TRUE(chars[2] == 'f');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** ']' after '^' in scanset does not lose meaning of scanset delimiter */
PCUT_TEST(set_inverted_with_only_dash)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abc-", "%[^-]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** '^' after '-' in scanset does not have special meaning */
PCUT_TEST(set_inverted_with_dash_caret)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("-^a", "%[-^a]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == '-');
	PCUT_ASSERT_TRUE(chars[1] == '^');
	PCUT_ASSERT_TRUE(chars[2] == 'a');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with range (GNU extension) */
PCUT_TEST(set_with_range)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("abc]", "%[a-c]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with range (GNU extension) in inverted scanset */
PCUT_TEST(set_inverted_with_range)
{
	int rc;
	char chars[chars_size];

	memset(chars, 'X', chars_size);
	rc = sscanf("defb", "%[^a-c]", chars);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'd');
	PCUT_ASSERT_TRUE(chars[1] == 'e');
	PCUT_ASSERT_TRUE(chars[2] == 'f');
	PCUT_ASSERT_TRUE(chars[3] == '\0');
	PCUT_ASSERT_TRUE(chars[4] == 'X');
}

/** Set conversion with assignment suppression */
PCUT_TEST(set_noassign)
{
	int rc;
	int n;

	rc = sscanf("abcd42", "%*[abc]%n", &n);
	PCUT_ASSERT_INT_EQUALS(0, rc);
	PCUT_ASSERT_INT_EQUALS(3, n);
}

/** Set conversion with memory allocation */
PCUT_TEST(set_malloc)
{
	int rc;
	char *cp;

	cp = NULL;
	rc = sscanf("abcd42", "%m[abcd]", &cp);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_NOT_NULL(cp);
	PCUT_ASSERT_TRUE(cp[0] == 'a');
	PCUT_ASSERT_TRUE(cp[1] == 'b');
	PCUT_ASSERT_TRUE(cp[2] == 'c');
	PCUT_ASSERT_TRUE(cp[3] == 'd');
	PCUT_ASSERT_TRUE(cp[4] == '\0');
	free(cp);
}

/** Decimal integer with suppressed assignment */
PCUT_TEST(dec_int_noassign)
{
	int rc;
	int n;

	rc = sscanf("42", "%*d%n", &n);
	PCUT_ASSERT_INT_EQUALS(0, rc);
	PCUT_ASSERT_INT_EQUALS(2, n);
}

/** Count of characters read */
PCUT_TEST(count_chars)
{
	int rc;
	char chars[chars_size];
	int n;

	memset(chars, 'X', chars_size);
	rc = sscanf("abcd", "%3c%n", chars, &n);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(chars[0] == 'a');
	PCUT_ASSERT_TRUE(chars[1] == 'b');
	PCUT_ASSERT_TRUE(chars[2] == 'c');
	PCUT_ASSERT_TRUE(chars[3] == 'X');
	PCUT_ASSERT_INT_EQUALS(3, n);
}

/** Float with just integer part */
PCUT_TEST(float_intpart_only)
{
	int rc;
	float f;

	rc = sscanf("42", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 42.0);
}

/** Double with just integer part */
PCUT_TEST(double_intpart_only)
{
	int rc;
	double d;

	rc = sscanf("42", "%lf", &d);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(d == 42.0);
}

/** Long double with just integer part */
PCUT_TEST(ldouble_intpart_only)
{
	int rc;
	long double ld;

	rc = sscanf("42", "%Lf", &ld);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(ld == 42.0);
}

/** Float with just hexadecimal integer part */
PCUT_TEST(float_hex_intpart_only)
{
	int rc;
	float f;

	rc = sscanf("0x2a", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 0x2a.0p0);
}

/** Float with sign and integer part */
PCUT_TEST(float_sign_intpart)
{
	int rc;
	float f;

	rc = sscanf("-42", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == -42.0);
}

/** Float with integer and fractional part */
PCUT_TEST(float_intpart_fract)
{
	int rc;
	float f;

	rc = sscanf("4.2", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	/* 1/10 is not exactly representable in binary floating point */
	PCUT_ASSERT_TRUE(f > 4.199);
	PCUT_ASSERT_TRUE(f < 4.201);
}

/** Float with integer part and unsigned exponent */
PCUT_TEST(float_intpart_exp)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float with integer part and positive exponent */
PCUT_TEST(float_intpart_posexp)
{
	int rc;
	float f;
	rc = sscanf("42e+1", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float with integer part and negative exponent */
PCUT_TEST(float_intpart_negexp)
{
	int rc;
	float f;

	rc = sscanf("42e-1", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	/* 1/10 is not exactly representable in binary floating point */
	PCUT_ASSERT_TRUE(f > 4.199);
	PCUT_ASSERT_TRUE(f < 4.201);
}

/** Float with integer, fractional parts and unsigned exponent */
PCUT_TEST(float_intpart_fract_exp)
{
	int rc;
	float f;

	rc = sscanf("4.2e1", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 42.0);
}

/** Hexadecimal float with integer and fractional part */
PCUT_TEST(hexfloat_intpart_fract)
{
	int rc;
	float f;

	rc = sscanf("0x2.a", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 0x2.ap0);
}

/** Hexadecimal float with integer part and unsigned exponent */
PCUT_TEST(hexfloat_intpart_exp)
{
	int rc;
	float f;

	rc = sscanf("0x2ap1", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 0x2ap1);
}

/** Hexadecimal float with integer part and negative exponent */
PCUT_TEST(hexfloat_intpart_negexp)
{
	int rc;
	float f;

	rc = sscanf("0x2ap-1", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 0x2ap-1);
}

/** Hexadecimal float with integer, fractional parts and unsigned exponent */
PCUT_TEST(hexfloat_intpart_fract_exp)
{
	int rc;
	float f;

	rc = sscanf("0x2.ap4", "%f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 0x2.ap4);
}

/** Float with just integer part and limited width */
PCUT_TEST(float_intpart_limwidth)
{
	int rc;
	float f;

	rc = sscanf("1234", "%3f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 123.0);
}

/** Float with integer, fractional part and limited width */
PCUT_TEST(float_intpart_fract_limwidth)
{
	int rc;
	float f;

	rc = sscanf("12.34", "%4f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	/* 1/10 is not exactly representable in binary floating point */
	PCUT_ASSERT_TRUE(f > 12.29);
	PCUT_ASSERT_TRUE(f < 12.31);
}

/** Float with width only enough to cover an integral part */
PCUT_TEST(float_width_for_only_intpart)
{
	int rc;
	float f;

	rc = sscanf("12.34", "%3f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 12.0);
}

/** Float with width too small to cover the exponent number */
PCUT_TEST(float_width_small_for_expnum)
{
	int rc;
	float f;

	rc = sscanf("12.34e+2", "%7f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	/* 1/10 is not exactly representable in binary floating point */
	PCUT_ASSERT_TRUE(f > 12.339);
	PCUT_ASSERT_TRUE(f < 12.341);
}

/** Float with width too small to cover the exponent sign and number */
PCUT_TEST(float_width_small_for_expsignum)
{
	int rc;
	float f;

	rc = sscanf("12.34e+2", "%6f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	/* 1/10 is not exactly representable in binary floating point */
	PCUT_ASSERT_TRUE(f > 12.339);
	PCUT_ASSERT_TRUE(f < 12.341);
}

/** Float with width too small to cover the exponent part */
PCUT_TEST(float_width_small_for_exp)
{
	int rc;
	float f;

	rc = sscanf("12.34e+2", "%5f", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	/* 1/10 is not exactly representable in binary floating point */
	PCUT_ASSERT_TRUE(f > 12.339);
	PCUT_ASSERT_TRUE(f < 12.341);
}

/** Float using alternate form 'F' */
PCUT_TEST(float_cap_f)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%F", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float using alternate form 'a' */
PCUT_TEST(float_a)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%a", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float using alternate form 'e' */
PCUT_TEST(float_e)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%e", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float using alternate form 'g' */
PCUT_TEST(float_g)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%g", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float using alternate form 'A' */
PCUT_TEST(float_cap_a)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%A", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float using alternate form 'E' */
PCUT_TEST(float_cap_e)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%E", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

/** Float using alternate form 'G' */
PCUT_TEST(float_cap_g)
{
	int rc;
	float f;

	rc = sscanf("42e1", "%G", &f);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_TRUE(f == 420.0);
}

PCUT_EXPORT(scanf);
