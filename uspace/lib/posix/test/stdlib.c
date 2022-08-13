/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
