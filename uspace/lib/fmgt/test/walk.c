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

#include <errno.h>
#include <fmgt.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <stdio.h>
#include <str.h>
#include <vfs/vfs.h>

PCUT_INIT;

PCUT_TEST_SUITE(walk);

static errno_t test_walk_dir_enter(void *, const char *);
static errno_t test_walk_dir_leave(void *, const char *);
static errno_t test_walk_file(void *, const char *);

static fmgt_walk_cb_t test_walk_cb = {
	.dir_enter = test_walk_dir_enter,
	.dir_leave = test_walk_dir_leave,
	.file = test_walk_file
};

typedef struct {
	bool dir_enter;
	bool dir_leave;
	bool file_proc;
	char *dirname;
	char *fname;
	errno_t rc;
} test_resp_t;

/** Walk file system tree. */
PCUT_TEST(walk_success)
{
	char buf[L_tmpnam];
	char *fname;
	FILE *f;
	char *p;
	int rv;
	fmgt_flist_t *flist;
	fmgt_walk_params_t params;
	test_resp_t resp;
	errno_t rc;

	/* Create name for temporary directory */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	/* Create temporary directory */
	rc = vfs_link_path(p, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = asprintf(&fname, "%s/%s", p, "a");
	PCUT_ASSERT_TRUE(rv >= 0);

	f = fopen(fname, "wb");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fprintf(f, "X");
	PCUT_ASSERT_TRUE(rv >= 0);

	rv = fclose(f);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rc = fmgt_flist_create(&flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_flist_append(flist, p);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	fmgt_walk_params_init(&params);
	params.flist = flist;
	params.cb = &test_walk_cb;
	params.arg = &resp;

	resp.dir_enter = false;
	resp.dir_leave = false;
	resp.file_proc = false;
	resp.rc = EOK;

	rc = fmgt_walk(&params);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(resp.dir_enter);
	PCUT_ASSERT_TRUE(resp.dir_leave);
	PCUT_ASSERT_TRUE(resp.file_proc);
	PCUT_ASSERT_STR_EQUALS(p, resp.dirname);
	PCUT_ASSERT_STR_EQUALS(fname, resp.fname);

	free(resp.dirname);
	free(resp.fname);
	fmgt_flist_destroy(flist);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(fname);
}

errno_t test_walk_dir_enter(void *arg, const char *fname)
{
	test_resp_t *resp = (test_resp_t *)arg;
	resp->dir_enter = true;
	resp->dirname = str_dup(fname);
	return resp->rc;
}

errno_t test_walk_dir_leave(void *arg, const char *fname)
{
	test_resp_t *resp = (test_resp_t *)arg;
	resp->dir_leave = true;
	return resp->rc;
}

errno_t test_walk_file(void *arg, const char *fname)
{
	test_resp_t *resp = (test_resp_t *)arg;
	resp->file_proc = true;
	resp->fname = str_dup(fname);
	return resp->rc;
}

PCUT_EXPORT(walk);
