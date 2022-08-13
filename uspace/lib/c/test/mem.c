/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mem.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(mem);

/** memcpy function */
PCUT_TEST(memcpy)
{
	char buf[5];
	void *p;

	p = memcpy(buf, "abc\0d", 5);
	PCUT_ASSERT_TRUE(p == buf);

	PCUT_ASSERT_INT_EQUALS('a', buf[0]);
	PCUT_ASSERT_INT_EQUALS('b', buf[1]);
	PCUT_ASSERT_INT_EQUALS('c', buf[2]);
	PCUT_ASSERT_INT_EQUALS('\0', buf[3]);
	PCUT_ASSERT_INT_EQUALS('d', buf[4]);
}

/** memmove function */
PCUT_TEST(memmove)
{
	char buf[] = "abc\0d";
	void *p;

	p = memmove(buf, buf + 1, 4);
	PCUT_ASSERT_TRUE(p == buf);

	PCUT_ASSERT_INT_EQUALS('b', buf[0]);
	PCUT_ASSERT_INT_EQUALS('c', buf[1]);
	PCUT_ASSERT_INT_EQUALS('\0', buf[2]);
	PCUT_ASSERT_INT_EQUALS('d', buf[3]);
	PCUT_ASSERT_INT_EQUALS('d', buf[4]);
}

/** memcmp function */
PCUT_TEST(memcmp)
{
	const char *s1 = "ab" "\0" "1d";
	const char *s2 = "ab" "\0" "2d";
	int c;

	c = memcmp(s1, s2, 3);
	PCUT_ASSERT_INT_EQUALS(0, c);

	c = memcmp(s1, s2, 4);
	PCUT_ASSERT_TRUE(c < 0);

	c = memcmp(s2, s1, 4);
	PCUT_ASSERT_TRUE(c > 0);
}

/** memchr function */
PCUT_TEST(memchr)
{
	const char *s = "abc\0d";
	void *p;

	p = memchr(s, 'd', 5);
	PCUT_ASSERT_TRUE(p == s + 4);

	p = memchr(s, '\0', 5);
	PCUT_ASSERT_TRUE(p == s + 3);

	p = memchr(s, 'd', 4);
	PCUT_ASSERT_TRUE(p == NULL);
}

/** memset function */
PCUT_TEST(memset)
{
	char buf[5];
	void *p;

	buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = 'a';
	p = memset(buf, 'x', 5);
	PCUT_ASSERT_TRUE(p == buf);

	PCUT_ASSERT_INT_EQUALS('x', buf[0]);
	PCUT_ASSERT_INT_EQUALS('x', buf[1]);
	PCUT_ASSERT_INT_EQUALS('x', buf[2]);
	PCUT_ASSERT_INT_EQUALS('x', buf[3]);
	PCUT_ASSERT_INT_EQUALS('x', buf[4]);
}

PCUT_EXPORT(mem);
