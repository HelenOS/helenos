/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <pcut/pcut.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

PCUT_INIT;

PCUT_TEST_SUITE(unistd);

#define MKSTEMP_TEMPL "/tmp/tmp.XXXXXX"
#define MKTEMP_BAD_TEMPL "/tmp/tmp.XXXXX"
#define MKTEMP_SHORT_TEMPL "XXXXX"

/** access function with nonexisting entry */
PCUT_TEST(access_nonexist)
{
	char name[L_tmpnam];
	char *p;
	int rc;

	p = tmpnam(name);
	PCUT_ASSERT_NOT_NULL(p);

	rc = access(name, F_OK);
	PCUT_ASSERT_INT_EQUALS(-1, rc);
}

/** access function with file */
PCUT_TEST(access_file)
{
	char name[L_tmpnam];
	char *p;
	int file;
	int rc;

	p = tmpnam(name);
	PCUT_ASSERT_NOT_NULL(p);

	file = open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
	PCUT_ASSERT_TRUE(file >= 0);

	rc = access(name, F_OK);
	PCUT_ASSERT_INT_EQUALS(0, rc);

	(void) unlink(name);
	close(file);
}

PCUT_EXPORT(unistd);
