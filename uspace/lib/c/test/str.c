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
	const char overlong1[] = { 0b11000000, 0b10000000, 0 };
	const char overlong2[] = { 0b11100000, 0b10000000, 0 };
	const char overlong3[] = { 0b11110000, 0b10000000, 0 };

	const char overlong4[] = { 0b11000001, 0b10111111, 0 };
	const char overlong5[] = { 0b11100000, 0b10011111, 0b10111111, 0 };
	const char overlong6[] = { 0b11110000, 0b10001111, 0b10111111, 0b10111111, 0 };

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

	char sanitized[sizeof(overlong6)];
	str_cpy(sanitized, STR_NO_LIMIT, overlong1);
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, sanitized[0]);
	str_cpy(sanitized, STR_NO_LIMIT, overlong2);
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, sanitized[0]);
	str_cpy(sanitized, STR_NO_LIMIT, overlong3);
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, sanitized[0]);
	str_cpy(sanitized, STR_NO_LIMIT, overlong4);
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, sanitized[0]);
	str_cpy(sanitized, STR_NO_LIMIT, overlong5);
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, sanitized[0]);
	str_cpy(sanitized, STR_NO_LIMIT, overlong6);
	PCUT_ASSERT_INT_EQUALS(U_SPECIAL, sanitized[0]);
}

PCUT_EXPORT(str);
