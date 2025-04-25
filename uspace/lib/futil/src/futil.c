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

/** @addtogroup futil
 * @{
 */
/** @file File manipulation utility functions
 */

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <vfs/vfs.h>
#include <dirent.h>

#include "futil.h"

#define BUF_SIZE 16384
static char buf[BUF_SIZE];

/** Create file utility instance.
 *
 * @param cb Callback functions
 * @param arg Argument to callback functions
 * @param rfutil Place to store pointer to new file utility instance
 * @return EOK on succcess, ENOMEM if out of memory.
 */
errno_t futil_create(futil_cb_t *cb, void *arg, futil_t **rfutil)
{
	futil_t *futil;

	futil = calloc(1, sizeof(futil_t));
	if (futil == NULL)
		return ENOMEM;

	futil->cb = cb;
	futil->cb_arg = arg;
	*rfutil = futil;
	return EOK;
}

/** Destroy file utility instance.
 *
 * @param futil File utility instance
 */
void futil_destroy(futil_t *futil)
{
	free(futil);
}

/** Copy file.
 *
 * @param futil File utility instance
 * @param srcp Source path
 * @param dstp Destination path
 *
 * @return EOK on success, EIO on I/O error
 */
errno_t futil_copy_file(futil_t *futil, const char *srcp, const char *destp)
{
	int sf, df;
	size_t nr, nw;
	errno_t rc;
	aoff64_t posr = 0, posw = 0;

	if (futil->cb != NULL && futil->cb->copy_file != NULL)
		futil->cb->copy_file(futil->cb_arg, srcp, destp);

	rc = vfs_lookup_open(srcp, WALK_REGULAR, MODE_READ, &sf);
	if (rc != EOK)
		return EIO;

	rc = vfs_lookup_open(destp, WALK_REGULAR | WALK_MAY_CREATE, MODE_WRITE,
	    &df);
	if (rc != EOK) {
		vfs_put(sf);
		return EIO;
	}

	do {
		rc = vfs_read(sf, &posr, buf, BUF_SIZE, &nr);
		if (rc != EOK)
			goto error;
		if (nr == 0)
			break;

		rc = vfs_write(df, &posw, buf, nr, &nw);
		if (rc != EOK)
			goto error;

	} while (nr == BUF_SIZE);

	(void) vfs_put(sf);

	rc = vfs_put(df);
	if (rc != EOK)
		return EIO;

	return EOK;
error:
	vfs_put(sf);
	vfs_put(df);
	return rc;
}

/** Copy contents of srcdir (recursively) into destdir.
 *
 * @param futil File utility instance
 * @param srcdir Source directory
 * @param destdir Destination directory
 *
 * @return EOK on success, ENOMEM if out of memory, EIO on I/O error
 */
errno_t futil_rcopy_contents(futil_t *futil, const char *srcdir,
    const char *destdir)
{
	DIR *dir;
	struct dirent *de;
	vfs_stat_t s;
	char *srcp, *destp;
	errno_t rc;

	dir = opendir(srcdir);
	if (dir == NULL)
		return EIO;

	de = readdir(dir);
	while (de != NULL) {
		if (asprintf(&srcp, "%s/%s", srcdir, de->d_name) < 0) {
			rc = ENOMEM;
			goto error;
		}

		if (asprintf(&destp, "%s/%s", destdir, de->d_name) < 0) {
			rc = ENOMEM;
			goto error;
		}

		rc = vfs_stat_path(srcp, &s);
		if (rc != EOK)
			goto error;

		if (s.is_file) {
			rc = futil_copy_file(futil, srcp, destp);
			if (rc != EOK) {
				rc = EIO;
				goto error;
			}
		} else if (s.is_directory) {
			if (futil->cb != NULL && futil->cb->create_dir != NULL)
				futil->cb->create_dir(futil->cb_arg, destp);
			rc = vfs_link_path(destp, KIND_DIRECTORY, NULL);
			if (rc != EOK)
				goto error;
			rc = futil_rcopy_contents(futil, srcp, destp);
			if (rc != EOK)
				goto error;
		} else {
			rc = EIO;
			goto error;
		}

		de = readdir(dir);
	}

	closedir(dir);
	return EOK;
error:
	closedir(dir);
	return rc;
}

/** Return file contents as a heap-allocated block of bytes.
 *
 * @param futil File utility instance
 * @param srcp File path
 * @param rdata Place to store pointer to data
 * @param rsize Place to store size of data
 *
 * @return EOK on success, ENOENT if failed to open file, EIO on other
 *         I/O error, ENOMEM if out of memory
 */
errno_t futil_get_file(futil_t *futil, const char *srcp, void **rdata,
    size_t *rsize)
{
	int sf;
	size_t nr;
	errno_t rc;
	size_t fsize;
	char *data;
	vfs_stat_t st;

	rc = vfs_lookup_open(srcp, WALK_REGULAR, MODE_READ, &sf);
	if (rc != EOK)
		return ENOENT;

	if (vfs_stat(sf, &st) != EOK) {
		vfs_put(sf);
		return EIO;
	}

	fsize = st.size;

	data = calloc(fsize, 1);
	if (data == NULL) {
		vfs_put(sf);
		return ENOMEM;
	}

	rc = vfs_read(sf, (aoff64_t []) { 0 }, data, fsize, &nr);
	if (rc != EOK || nr != fsize) {
		vfs_put(sf);
		free(data);
		return EIO;
	}

	(void) vfs_put(sf);
	*rdata = data;
	*rsize = fsize;

	return EOK;
}

/** @}
 */
