/*
 * Copyright (c) 2026 Jiri Svoboda
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

#include <fmgt.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <str.h>
#include <vfs/vfs.h>

PCUT_INIT;

PCUT_TEST_SUITE(newdir);

#define TEMP_DIR "/tmp"

/** Suggesting new directory name succeeds and returns unique name. */
PCUT_TEST(new_dir_suggest)
{
	char *dname1 = NULL;
	char *dname2 = NULL;
	errno_t rc;
	int rv;

	rc = vfs_cwd_set(TEMP_DIR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Suggest unique directory name. */
	rc = fmgt_new_dir_suggest(&dname1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* See if we can actually create the directory. */
	rc = vfs_link_path(dname1, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now suggest another unique directory name. */
	rc = fmgt_new_dir_suggest(&dname2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* They should be different. */
	PCUT_ASSERT_FALSE(str_cmp(dname1, dname2) == 0);

	/* Remove the directory. */
	rv = remove(dname1);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(dname1);
	free(dname2);
}

/** New directory can be created. */
PCUT_TEST(new_dir)
{
	fmgt_t *fmgt = NULL;
	char *dname1 = NULL;
	errno_t rc;
	int rv;

	rc = vfs_cwd_set(TEMP_DIR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Suggest unique directory name. */
	rc = fmgt_new_dir_suggest(&dname1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* See if we can actually create the directory. */
	rc = fmgt_new_dir(fmgt, dname1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Remove the directory. */
	rv = remove(dname1);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(dname1);
	fmgt_destroy(fmgt);
}

PCUT_EXPORT(newdir);
