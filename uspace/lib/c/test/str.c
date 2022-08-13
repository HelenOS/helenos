/*
 * SPDX-FileCopyrightText: 2015 Michal Koutny
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

PCUT_EXPORT(str);
