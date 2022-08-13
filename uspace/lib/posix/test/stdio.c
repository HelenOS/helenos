/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <str.h>
#include <stdio.h>

PCUT_INIT;

PCUT_TEST_SUITE(stdio);

/** tempnam function with directory argument not having trailing slash */
PCUT_TEST(tempnam_no_slash)
{
	char *p;
	FILE *f;

	p = tempnam("/tmp", "tmp.");
	PCUT_ASSERT_NOT_NULL(p);

	PCUT_ASSERT_TRUE(str_lcmp(p, "/tmp/tmp.",
	    str_length("/tmp/tmp.")) == 0);

	f = fopen(p, "w+x");
	PCUT_ASSERT_NOT_NULL(f);

	(void) remove(p);
	fclose(f);
}

/** tempnam function with directory argument having trailing slash */
PCUT_TEST(tempnam_with_slash)
{
	char *p;
	FILE *f;

	p = tempnam("/tmp/", "tmp.");
	PCUT_ASSERT_NOT_NULL(p);

	PCUT_ASSERT_TRUE(str_lcmp(p, "/tmp/tmp.",
	    str_length("/tmp/tmp.")) == 0);

	f = fopen(p, "w+x");
	PCUT_ASSERT_NOT_NULL(f);

	(void) remove(p);
	fclose(f);
}

/** tempnam function with NULL directory argument */
PCUT_TEST(tempnam_no_dir)
{
	char *p;
	FILE *f;

	p = tempnam(NULL, "tmp.");
	PCUT_ASSERT_NOT_NULL(p);

	PCUT_ASSERT_TRUE(str_lcmp(p, P_tmpdir "/tmp.",
	    str_length(P_tmpdir "/tmp.")) == 0);

	f = fopen(p, "w+x");
	PCUT_ASSERT_NOT_NULL(f);

	(void) remove(p);
	fclose(f);
}

PCUT_EXPORT(stdio);
