/*
 * Copyright (c) 2025 Jiri Svoboda
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

PCUT_TEST_SUITE(fmgt);

#define TEMP_DIR "/tmp"

static void test_fmgt_progress(void *, fmgt_progress_t *);

static fmgt_cb_t test_fmgt_cb = {
	.progress = test_fmgt_progress
};

typedef struct {
	unsigned nupdates;
} test_resp_t;

/** Create and destroy file management object succeeds. */
PCUT_TEST(create_destroy)
{
	fmgt_t *fmgt = NULL;
	errno_t rc;

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(fmgt);

	fmgt_destroy(fmgt);
}

/** Suggesting new file name succeeds and returns unique name. */
PCUT_TEST(new_file_suggest)
{
	FILE *f = NULL;
	char *fname1 = NULL;
	char *fname2 = NULL;
	errno_t rc;
	int rv;

	rc = vfs_cwd_set(TEMP_DIR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Suggest unique file name. */
	rc = fmgt_new_file_suggest(&fname1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* See if we can actually create the file. */
	f = fopen(fname1, "wx");
	PCUT_ASSERT_NOT_NULL(f);
	(void) fclose(f);

	/* Now suggest another unique file name. */
	rc = fmgt_new_file_suggest(&fname2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* They should be different. */
	PCUT_ASSERT_FALSE(str_cmp(fname1, fname2) == 0);

	/* Remove the file. */
	rv = remove(fname1);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(fname1);
	free(fname2);
}

/** New empty file can be created. */
PCUT_TEST(new_file_empty)
{
	fmgt_t *fmgt = NULL;
	char *fname = NULL;
	errno_t rc;
	int rv;

	rc = vfs_cwd_set(TEMP_DIR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Suggest unique file name. */
	rc = fmgt_new_file_suggest(&fname);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_new_file(fmgt, fname, 0, nf_none);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Remove the file [this also verifies the file exists]. */
	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(fname);
	fmgt_destroy(fmgt);
}

/** New zero-filled file can be created. */
PCUT_TEST(new_file_zerofill)
{
	FILE *f = NULL;
	fmgt_t *fmgt = NULL;
	char *fname = NULL;
	char buf[128];
	errno_t rc;
	size_t nr;
	size_t total_rd;
	size_t i;
	int rv;

	rc = vfs_cwd_set(TEMP_DIR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Suggest unique file name. */
	rc = fmgt_new_file_suggest(&fname);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_new_file(fmgt, fname, 20000, nf_none);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Verify the file. */
	f = fopen(fname, "rb");
	PCUT_ASSERT_NOT_NULL(f);
	total_rd = 0;
	do {
		nr = fread(buf, 1, sizeof(buf), f);
		for (i = 0; i < nr; i++)
			PCUT_ASSERT_INT_EQUALS(0, buf[i]);

		total_rd += nr;
	} while (nr > 0);

	PCUT_ASSERT_INT_EQUALS(20000, total_rd);

	(void)fclose(f);

	/* Remove the file. */
	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(fname);
	fmgt_destroy(fmgt);
}

/** New sparse file can be created. */
PCUT_TEST(new_file_sparse)
{
	FILE *f = NULL;
	fmgt_t *fmgt = NULL;
	char *fname = NULL;
	char buf[128];
	errno_t rc;
	size_t nr;
	size_t total_rd;
	size_t i;
	int rv;

	rc = vfs_cwd_set(TEMP_DIR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Suggest unique file name. */
	rc = fmgt_new_file_suggest(&fname);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_new_file(fmgt, fname, 20000, nf_sparse);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Verify the file. */
	f = fopen(fname, "rb");
	PCUT_ASSERT_NOT_NULL(f);
	total_rd = 0;
	do {
		nr = fread(buf, 1, sizeof(buf), f);
		for (i = 0; i < nr; i++)
			PCUT_ASSERT_INT_EQUALS(0, buf[i]);

		total_rd += nr;
	} while (nr > 0);

	PCUT_ASSERT_INT_EQUALS(20000, total_rd);

	(void)fclose(f);

	/* Remove the file. */
	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(fname);
	fmgt_destroy(fmgt);
}

/** Initial update provided when requested while creating new file. */
PCUT_TEST(new_file_init_upd)
{
	FILE *f = NULL;
	fmgt_t *fmgt = NULL;
	char *fname = NULL;
	char buf[128];
	errno_t rc;
	size_t nr;
	size_t total_rd;
	size_t i;
	test_resp_t resp;
	int rv;

	rc = vfs_cwd_set(TEMP_DIR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	fmgt_set_cb(fmgt, &test_fmgt_cb, (void *)&resp);
	resp.nupdates = 0;

	fmgt_set_init_update(fmgt, true);

	/* Suggest unique file name. */
	rc = fmgt_new_file_suggest(&fname);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_new_file(fmgt, fname, 20000, nf_none);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Verify the file. */
	f = fopen(fname, "rb");
	PCUT_ASSERT_NOT_NULL(f);
	total_rd = 0;
	do {
		nr = fread(buf, 1, sizeof(buf), f);
		for (i = 0; i < nr; i++)
			PCUT_ASSERT_INT_EQUALS(0, buf[i]);

		total_rd += nr;
	} while (nr > 0);

	PCUT_ASSERT_INT_EQUALS(20000, total_rd);

	(void)fclose(f);

	PCUT_ASSERT_TRUE(resp.nupdates > 0);

	/* Remove the file. */
	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(fname);
	fmgt_destroy(fmgt);
}

static void test_fmgt_progress(void *arg, fmgt_progress_t *progress)
{
	test_resp_t *resp = (test_resp_t *)arg;

	(void)progress;
	++resp->nupdates;
}

PCUT_EXPORT(fmgt);
