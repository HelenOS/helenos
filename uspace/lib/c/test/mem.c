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
