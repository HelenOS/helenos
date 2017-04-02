/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup sysinst
 * @{
 */
/** @file File manipulation utility functions for installer
 */

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <sys/types.h>
#include <dirent.h>

#include "futil.h"

#define BUF_SIZE 16384
static char buf[BUF_SIZE];

/** Copy file.
 *
 * @param srcp Source path
 * @param dstp Destination path
 *
 * @return EOK on success, EIO on I/O error
 */
int futil_copy_file(const char *srcp, const char *destp)
{
	int sf, df;
	ssize_t nr, nw;
	int rc;
	aoff64_t posr = 0, posw = 0;

	printf("Copy '%s' to '%s'.\n", srcp, destp);

	sf = vfs_lookup_open(srcp, WALK_REGULAR, MODE_READ);
	if (sf < 0)
		return EIO;

	df = vfs_lookup_open(destp, WALK_REGULAR | WALK_MAY_CREATE, MODE_WRITE);
	if (df < 0)
		return EIO;

	do {
		nr = read(sf, &posr, buf, BUF_SIZE);
		if (nr == 0)
			break;
		if (nr < 0)
			return EIO;

		nw = write(df, &posw, buf, nr);
		if (nw <= 0)
			return EIO;
	} while (true);

	(void) vfs_put(sf);

	rc = vfs_put(df);
	if (rc < 0)
		return EIO;

	return EOK;
}

/** Copy contents of srcdir (recursively) into destdir.
 *
 * @param srcdir Source directory
 * @param destdir Destination directory
 *
 * @return EOK on success, ENOMEM if out of memory, EIO on I/O error
 */
int futil_rcopy_contents(const char *srcdir, const char *destdir)
{
	DIR *dir;
	struct dirent *de;
	struct stat s;
	char *srcp, *destp;
	int rc;

	dir = opendir(srcdir);
	if (dir == NULL)
		return EIO;

	de = readdir(dir);
	while (de != NULL) {
		if (asprintf(&srcp, "%s/%s", srcdir, de->d_name) < 0)
			return ENOMEM;
		if (asprintf(&destp, "%s/%s", destdir, de->d_name) < 0)
			return ENOMEM;

		rc = vfs_stat_path(srcp, &s);
		if (rc != EOK)
			return EIO;

		if (s.is_file) {
			rc = futil_copy_file(srcp, destp);
			if (rc != EOK)
				return EIO;
		} else if (s.is_directory) {
			printf("Create directory '%s'\n", destp);
			rc = vfs_link_path(destp, KIND_DIRECTORY);
			if (rc != EOK)
				return EIO;
			rc = futil_rcopy_contents(srcp, destp);
			if (rc != EOK)
				return EIO;
		} else {
			return EIO;
		}

		de = readdir(dir);
	}

	return EOK;
}

/** Return file contents as a heap-allocated block of bytes.
 *
 * @param srcp File path
 * @param rdata Place to store pointer to data
 * @param rsize Place to store size of data
 *
 * @return EOK on success, ENOENT if failed to open file, EIO on other
 *         I/O error, ENOMEM if out of memory
 */
int futil_get_file(const char *srcp, void **rdata, size_t *rsize)
{
	int sf;
	ssize_t nr;
	size_t fsize;
	char *data;
	struct stat st;

	sf = vfs_lookup_open(srcp, WALK_REGULAR, MODE_READ);
	if (sf < 0)
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

	nr = read(sf, (aoff64_t []) { 0 }, data, fsize);
	if (nr != (ssize_t)fsize) {
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
