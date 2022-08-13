/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Test stdio module
 */

#include <errno.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <str.h>
#include <tmpfile.h>
#include <vfs/vfs.h>

PCUT_INIT;

PCUT_TEST_SUITE(stdio);

/** remove function */
PCUT_TEST(remove)
{
	char buf[L_tmpnam];
	char *p;
	FILE *f;
	int rc;

	/* Generate unique file name */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	/* Removing it should fail */
	rc = remove(buf);
	PCUT_ASSERT_TRUE(rc != 0);

	f = fopen(buf, "wx");
	PCUT_ASSERT_NOT_NULL(f);
	fclose(f);

	/* Remove for the first time */
	rc = remove(buf);
	PCUT_ASSERT_INT_EQUALS(0, rc);

	/* Should fail the second time */
	rc = remove(buf);
	PCUT_ASSERT_TRUE(rc != 0);
}

/** rename function */
PCUT_TEST(rename)
{
	char buf1[L_tmpnam];
	char buf2[L_tmpnam];
	char *p;
	FILE *f;
	int rc;

	/* Generate first unique file name */
	p = tmpnam(buf1);
	PCUT_ASSERT_NOT_NULL(p);

	/* Generate second unique file name */
	p = tmpnam(buf2);
	PCUT_ASSERT_NOT_NULL(p);

	f = fopen(buf1, "wx");
	PCUT_ASSERT_NOT_NULL(f);
	fclose(f);

	/* Rename to the second name */
	rc = rename(buf1, buf2);
	PCUT_ASSERT_INT_EQUALS(0, rc);

	/* First name should no longer exist */
	rc = remove(buf1);
	PCUT_ASSERT_TRUE(rc != 0);

	/* Second can be removed */
	rc = remove(buf2);
	PCUT_ASSERT_INT_EQUALS(0, rc);
}

/** tmpfile function */
PCUT_TEST(tmpfile)
{
	FILE *f;
	char c;
	size_t n;

	f = tmpfile();
	PCUT_ASSERT_NOT_NULL(f);

	c = 'x';
	n = fwrite(&c, sizeof(c), 1, f);
	PCUT_ASSERT_INT_EQUALS(1, n);

	rewind(f);
	c = '\0';
	n = fread(&c, sizeof(c), 1, f);
	PCUT_ASSERT_INT_EQUALS(1, n);
	PCUT_ASSERT_INT_EQUALS('x', c);

	fclose(f);
}

/** tmpnam function with buffer argument */
PCUT_TEST(tmpnam_buf)
{
	char buf[L_tmpnam];
	char *p;
	FILE *f;

	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	f = fopen(p, "w+x");
	PCUT_ASSERT_NOT_NULL(f);
	(void) remove(p);
	fclose(f);
}

/** tmpnam function called twice */
PCUT_TEST(tmpnam_twice)
{
	char buf1[L_tmpnam];
	char buf2[L_tmpnam];
	char *p;

	p = tmpnam(buf1);
	PCUT_ASSERT_NOT_NULL(p);

	p = tmpnam(buf2);
	PCUT_ASSERT_NOT_NULL(p);

	/* We must get two distinct names */
	PCUT_ASSERT_TRUE(str_cmp(buf1, buf2) != 0);
}

/** tmpnam function with NULL argument */
PCUT_TEST(tmpnam_null)
{
	char *p;
	FILE *f;

	p = tmpnam(NULL);
	PCUT_ASSERT_NOT_NULL(p);

	f = fopen(p, "w+x");
	PCUT_ASSERT_NOT_NULL(f);
	(void) remove(p);
	fclose(f);
}

/** fgetpos and fsetpos functions */
PCUT_TEST(fgetpos_setpos)
{
	fpos_t pos;
	int rc;
	FILE *f;

	f = tmpfile();
	PCUT_ASSERT_NOT_NULL(f);

	rc = fputs("abc", f);
	PCUT_ASSERT_TRUE(rc >= 0);

	rc = fgetpos(f, &pos);
	PCUT_ASSERT_TRUE(rc >= 0);

	rewind(f);

	rc = fsetpos(f, &pos);
	PCUT_ASSERT_TRUE(rc >= 0);

	(void) fclose(f);
}

/** perror function with NULL as argument */
PCUT_TEST(perror_null_msg)
{
	errno = EINVAL;
	perror(NULL);
}

/** perror function with empty string as argument */
PCUT_TEST(perror_empty_msg)
{
	errno = EINVAL;
	perror("");
}

/** perror function with a message argument */
PCUT_TEST(perror_msg)
{
	errno = EINVAL;
	perror("This is a test");
}

PCUT_EXPORT(stdio);
