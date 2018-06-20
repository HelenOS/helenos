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

PCUT_INIT;

PCUT_TEST_SUITE(stdio);

/** fgetpos and fsetpos functions */
PCUT_TEST(fgetpos_setpos)
{
	fpos_t pos;
	int rc;
	FILE *f;

	// XXX Use tmpfile() when it is available
	f = fopen("/tmp/fgpsp.tmp", "w+");
	PCUT_ASSERT_NOT_NULL(f);
	rc = remove("/tmp/fgpsp.tmp");
	PCUT_ASSERT_INT_EQUALS(0, rc);

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
