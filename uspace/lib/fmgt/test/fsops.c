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

#include <errno.h>
#include <fmgt.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <stdio.h>
#include <str.h>
#include <vfs/vfs.h>

#include "../private/fsops.h"

PCUT_INIT;

PCUT_TEST_SUITE(fsops);

/** Open file. */
PCUT_TEST(open)
{
	fmgt_t *fmgt = NULL;
	char buf[L_tmpnam];
	FILE *f;
	char *p;
	int fd;
	int rv;
	errno_t rc;

	/* Create name for temporary file */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	f = fopen(p, "wb");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fprintf(f, "X");
	PCUT_ASSERT_TRUE(rv >= 0);

	rv = fclose(f);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_open(fmgt, p, &fd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	fmgt_destroy(fmgt);
	vfs_put(fd);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Create file. */
PCUT_TEST(create_file)
{
	fmgt_t *fmgt = NULL;
	char buf[L_tmpnam];
	char *p;
	int fd;
	int rv;
	fmgt_exists_action_t exaction;
	errno_t rc;

	/* Create name for temporary file */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create_file(fmgt, p, &fd, &exaction);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	fmgt_destroy(fmgt);
	vfs_put(fd);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Create directory. */
PCUT_TEST(create_dir)
{
	fmgt_t *fmgt = NULL;
	char buf[L_tmpnam];
	char *p;
	int rv;
	errno_t rc;

	/* Create name for temporary directory */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create_dir(fmgt, p, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	fmgt_destroy(fmgt);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Remove file. */
PCUT_TEST(remove)
{
	fmgt_t *fmgt = NULL;
	char buf[L_tmpnam];
	FILE *f;
	char *p;
	int rv;
	errno_t rc;

	/* Create name for temporary file */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	f = fopen(p, "wb");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fprintf(f, "X");
	PCUT_ASSERT_TRUE(rv >= 0);

	rv = fclose(f);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_remove(fmgt, p);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	fmgt_destroy(fmgt);
}

/** Read data from file. */
PCUT_TEST(read)
{
	fmgt_t *fmgt = NULL;
	char buf[L_tmpnam];
	char dbuffer[64];
	FILE *f;
	char *p;
	int fd;
	int rv;
	aoff64_t pos;
	size_t nr;
	errno_t rc;

	/* Create name for temporary file */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	f = fopen(p, "wb");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fprintf(f, "XYZ");
	PCUT_ASSERT_TRUE(rv >= 0);

	(void)fclose(f);

	rc = vfs_lookup_open(p, WALK_REGULAR, MODE_READ, &fd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos = 0;
	rc = fmgt_read(fmgt, fd, p, &pos, dbuffer, sizeof(dbuffer), &nr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(3, nr);
	PCUT_ASSERT_INT_EQUALS(3, pos);

	fmgt_destroy(fmgt);
	vfs_put(fd);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);

}

/** Write data to file. */
PCUT_TEST(write)
{

	fmgt_t *fmgt = NULL;
	char buf[L_tmpnam];
	char dbuffer[64];
	char data[3] = "XYZ";
	FILE *f;
	char *p;
	int fd;
	int rv;
	aoff64_t pos;
	size_t nr;
	errno_t rc;

	/* Create name for temporary file */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	rc = fmgt_create(&fmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = vfs_lookup_open(p, WALK_REGULAR | WALK_MUST_CREATE, MODE_WRITE,
	    &fd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos = 0;
	rc = fmgt_write(fmgt, fd, p, &pos, data, sizeof(data));
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(pos, 3);

	fmgt_destroy(fmgt);
	vfs_put(fd);

	f = fopen(p, "rb");
	PCUT_ASSERT_NOT_NULL(f);

	nr = fread(dbuffer, 1, sizeof(dbuffer), f);
	PCUT_ASSERT_INT_EQUALS(nr, 3);
	(void)fclose(f);

	PCUT_ASSERT_TRUE(memcmp(data, dbuffer, sizeof(data)) == 0);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

PCUT_EXPORT(fsops);
