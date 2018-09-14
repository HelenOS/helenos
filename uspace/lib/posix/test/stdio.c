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
