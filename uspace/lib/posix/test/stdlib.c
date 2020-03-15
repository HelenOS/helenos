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

#include <pcut/pcut.h>
#include <str.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

PCUT_INIT;

PCUT_TEST_SUITE(stdlib);

#define MKSTEMP_TEMPL "/tmp/tmp.XXXXXX"
#define MKTEMP_BAD_TEMPL "/tmp/tmp.XXXXX"
#define MKTEMP_SHORT_TEMPL "XXXXX"

/** mktemp function */
PCUT_TEST(mktemp)
{
	int file;
	char buf[sizeof(MKSTEMP_TEMPL)];
	char *p;

	str_cpy(buf, sizeof(buf), MKSTEMP_TEMPL);

	p = mktemp(buf);
	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(str_lcmp(p, MKSTEMP_TEMPL,
	    str_length(MKSTEMP_TEMPL) - 6) == 0);

	file = open(p, O_CREAT | O_EXCL | O_RDWR, 0600);
	PCUT_ASSERT_TRUE(file >= 0);
	close(file);

	(void) unlink(p);
}

/** mktemp function called twice should return different names */
PCUT_TEST(mktemp_twice)
{
	char buf1[sizeof(MKSTEMP_TEMPL)];
	char buf2[sizeof(MKSTEMP_TEMPL)];
	char *p;

	str_cpy(buf1, sizeof(buf1), MKSTEMP_TEMPL);
	str_cpy(buf2, sizeof(buf2), MKSTEMP_TEMPL);

	p = mktemp(buf1);
	PCUT_ASSERT_TRUE(p == buf1);
	PCUT_ASSERT_TRUE(str_lcmp(p, MKSTEMP_TEMPL,
	    str_length(MKSTEMP_TEMPL) - 6) == 0);

	p = mktemp(buf2);
	PCUT_ASSERT_TRUE(p == buf2);
	PCUT_ASSERT_TRUE(str_lcmp(p, MKSTEMP_TEMPL,
	    str_length(MKSTEMP_TEMPL) - 6) == 0);

	PCUT_ASSERT_TRUE(str_cmp(buf1, buf2) != 0);
}

/** mktemp function with malformed template */
PCUT_TEST(mktemp_bad_templ)
{
	char buf[sizeof(MKTEMP_BAD_TEMPL)];
	char *p;

	str_cpy(buf, sizeof(buf), MKTEMP_BAD_TEMPL);

	p = mktemp(buf);
	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(p[0] == '\0');
}

/** mktemp function with too short template */
PCUT_TEST(mktemp_short_templ)
{
	char buf[sizeof(MKTEMP_SHORT_TEMPL)];
	char *p;

	str_cpy(buf, sizeof(buf), MKTEMP_SHORT_TEMPL);

	p = mktemp(buf);
	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(p[0] == '\0');
}

/** mkstemp function */
PCUT_TEST(mkstemp)
{
	int file;
	char buf[sizeof(MKSTEMP_TEMPL)];
	ssize_t rc;
	char c;

	str_cpy(buf, sizeof(buf), MKSTEMP_TEMPL);

	file = mkstemp(buf);
	PCUT_ASSERT_TRUE(file >= 0);

	(void) unlink(buf);

	c = 'x';
	rc = write(file, &c, sizeof(c));
	PCUT_ASSERT_TRUE(rc == sizeof(c));

	(void) lseek(file, 0, SEEK_SET);
	c = '\0';
	rc = read(file, &c, sizeof(c));
	PCUT_ASSERT_TRUE(rc == sizeof(c));
	PCUT_ASSERT_INT_EQUALS('x', c);

	close(file);
}

PCUT_EXPORT(stdlib);
