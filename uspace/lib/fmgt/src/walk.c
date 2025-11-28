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

/** @addtogroup fmgt
 * @{
 */
/** @file File system tree walker.
 */

#include <adt/list.h>
#include <dirent.h>
#include <errno.h>
#include <mem.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <vfs/vfs.h>

#include "fmgt.h"
#include "fmgt/walk.h"

static errno_t fmgt_walk_subtree(fmgt_walk_params_t *, const char *);

/** Initialize walk parameters.
 *
 * Every walk parameters structure must be initialized first using this
 * function.
 *
 * @param params Pointer to walk parameters structure.
 */
void fmgt_walk_params_init(fmgt_walk_params_t *params)
{
	memset(params, 0, sizeof(fmgt_walk_params_t));
}

/** Walk file (invoke file callback).
 *
 * @param params Walk parameters
 * @param fname File path
 *
 * @return EOK on success or an error code
 */
static errno_t fmgt_walk_file(fmgt_walk_params_t *params, const char *fname)
{
	if (params->cb->file != NULL)
		return params->cb->file(params->arg, fname);
	else
		return EOK;
}

/** Enter directory (invoke directory entry callback).
 *
 * @param params Walk parameters
 * @param dname Directory path
 * @return EOK on success or an error code
 */
static errno_t fmgt_walk_dir_enter(fmgt_walk_params_t *params,
    const char *dname)
{
	if (params->cb->dir_enter != NULL)
		return params->cb->dir_enter(params->arg, dname);
	else
		return EOK;
}

/** Leave directory (invoke directory exit callback).
 *
 * @param params Walk parameters
 * @param dname Directory path
 * @return EOK on success or an error code
 */
static errno_t fmgt_walk_dir_leave(fmgt_walk_params_t *params,
    const char *dname)
{
	if (params->cb->dir_enter != NULL)
		return params->cb->dir_leave(params->arg, dname);
	else
		return EOK;
}

/** Walk directory.
 *
 * @param params Walk parameters
 * @param dname Directory name
 * @return EOK on success or an error code
 */
static errno_t fmgt_walk_dir(fmgt_walk_params_t *params, const char *dname)
{
	DIR *dir = NULL;
	struct dirent *de;
	errno_t rc;
	char *pathname = NULL;
	int rv;

	rc = fmgt_walk_dir_enter(params, dname);
	if (rc != EOK)
		goto error;

	dir = opendir(dname);
	if (dir == NULL) {
		rc = EIO;
		goto error;
	}

	de = readdir(dir);
	while (de != NULL) {
		rv = asprintf(&pathname, "%s/%s", dname, de->d_name);
		if (rv < 0) {
			rc = ENOMEM;
			goto error;
		}

		rc = fmgt_walk_subtree(params, pathname);
		if (rc != EOK) {
			free(pathname);
			goto error;
		}

		free(pathname);
		de = readdir(dir);
	}

	rc = fmgt_walk_dir_leave(params, dname);
	if (rc != EOK)
		return rc;

	closedir(dir);
	return EOK;
error:
	if (dir != NULL)
		closedir(dir);
	return rc;
}

/** Walk subtree.
 *
 * @params params Walk parameters.
 * @params fname Subtree path.
 *
 * @return EOK on success or an error code.
 */
static errno_t fmgt_walk_subtree(fmgt_walk_params_t *params, const char *fname)
{
	vfs_stat_t stat;
	errno_t rc;

	rc = vfs_stat_path(fname, &stat);
	if (rc != EOK)
		return rc;

	if (stat.is_directory) {
		/* Directory */
		rc = fmgt_walk_dir(params, fname);
		if (rc != EOK)
			return rc;
	} else {
		/* Not a directory */
		rc = fmgt_walk_file(params, fname);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Perform a file system walk.
 *
 * Walks the list of files/directories in @a params->flist. Directories
 * are walked recursively. Callbacks are involved for each file, directory,
 * if defined.in @a params->cb, the callback argument is @a params->arg.
 *
 * @param params Walk parameters
 * @return EOK on success or an error code.
 */
errno_t fmgt_walk(fmgt_walk_params_t *params)
{
	fmgt_flist_entry_t *entry;
	errno_t rc;

	entry = fmgt_flist_first(params->flist);
	while (entry != NULL) {
		rc = fmgt_walk_subtree(params, entry->fname);
		if (rc != EOK)
			return rc;

		entry = fmgt_flist_next(entry);
	}

	return EOK;
}

/** @}
 */
