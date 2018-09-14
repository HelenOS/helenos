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
